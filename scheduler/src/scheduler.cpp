#include "scheduler.h"
#include "job_dao.h"
#include "kafka_message_queue.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <random>
#include <algorithm>
#include "cron_parser.h"

namespace scheduler
{

  // 生成UUID
  std::string generate_uuid()
  {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char *chars = "0123456789abcdef";

    std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";

    for (char &c : uuid)
    {
      if (c == 'x')
      {
        c = chars[dis(gen)];
      }
      else if (c == 'y')
      {
        c = chars[(dis(gen) & 0x3) | 0x8];
      }
    }

    return uuid;
  }

  // 任务队列类
  class JobQueue
  {
  public:
    JobQueue() = default;

    // 添加任务
    void push(const JobInfo &job)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      jobs_.push_back(job);
      // 按优先级排序，高优先级在前
      std::sort(jobs_.begin(), jobs_.end(), [](const JobInfo &a, const JobInfo &b)
                { return a.priority > b.priority; });
    }

    // 获取下一个任务
    std::optional<JobInfo> pop()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (jobs_.empty())
      {
        return std::nullopt;
      }
      JobInfo job = jobs_.front();
      jobs_.erase(jobs_.begin());
      return job;
    }

    // 移除指定任务
    bool remove(const std::string &job_id)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = std::find_if(jobs_.begin(), jobs_.end(), [&job_id](const JobInfo &job)
                             { return job.job_id == job_id; });
      if (it != jobs_.end())
      {
        jobs_.erase(it);
        return true;
      }
      return false;
    }

    // 获取任务数量
    size_t size() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return jobs_.size();
    }

  private:
    std::vector<JobInfo> jobs_;
    mutable std::mutex mutex_;
  };

  // 执行器注册表类
  class ExecutorRegistry
  {
  public:
    ExecutorRegistry(JobDAO &dao) : dao_(dao) {}

    // 获取可用执行器
    std::optional<std::pair<std::string, std::string>> getAvailableExecutor()
    {
      auto executors = dao_.getOnlineExecutors();
      if (executors.empty())
      {
        return std::nullopt;
      }

      // 随机选择一个执行器
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, executors.size() - 1);
      int index = dis(gen);

      return executors[index];
    }

  private:
    JobDAO &dao_;
  };

  // JobScheduler实现
  JobScheduler::JobScheduler()
      : running_(false)
  {
    // 初始化数据库连接池
    auto &dbPool = DBConnectionPool::getInstance();
    dbPool.initialize("localhost", "root", "password", "distributed_scheduler");

    // 创建组件
    job_storage_ = std::make_unique<JobDAO>();
    job_queue_ = std::make_unique<JobQueue>();
    executor_registry_ = std::make_unique<ExecutorRegistry>(*job_storage_);
    kafka_client_ = std::make_unique<KafkaMessageQueue>();

    // 初始化Kafka
    kafka_client_->initProducer("localhost:9092");
    kafka_client_->initConsumer("localhost:9092", "scheduler-group", {"job-result"}, [this](const KafkaMessage &message)
                                {
          if (message.type == MessageType::JOB_RESULT)
          {
            try
            {
              // 解析任务结果
              nlohmann::json j = nlohmann::json::parse(message.payload);
              JobResult result;
              result.from_json(j);
              
              // 处理任务结果
              handle_result(result);
            }
            catch (const std::exception &e)
            {
              spdlog::error("Failed to parse job result: {}", e.what());
            }
          } });
  }

  JobScheduler::~JobScheduler()
  {
    stop();
  }

  void JobScheduler::start()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_)
    {
      return;
    }

    running_ = true;
    schedule_thread_ = std::thread(&JobScheduler::schedule_loop, this);

    // 启动Kafka消费
    kafka_client_->startConsume();

    spdlog::info("Job scheduler started");
  }

  void JobScheduler::stop()
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!running_)
      {
        return;
      }

      running_ = false;
      cv_.notify_all();
    }

    // 等待调度线程结束
    if (schedule_thread_.joinable())
    {
      schedule_thread_.join();
    }

    // 停止Kafka消费
    kafka_client_->stopConsume();

    spdlog::info("Job scheduler stopped");
  }

  std::string JobScheduler::submit_job(const JobInfo &job)
  {
    // 创建新任务
    JobInfo new_job = job;
    new_job.job_id = generate_uuid();

    // 保存到数据库
    if (!job_storage_->saveJob(new_job))
    {
      spdlog::error("Failed to save job to database");
      return "";
    }

    // 添加到任务队列
    job_queue_->push(new_job);

    // 通知调度线程
    cv_.notify_one();

    spdlog::info("Job submitted: {}", new_job.job_id);
    return new_job.job_id;
  }

  bool JobScheduler::cancel_job(const std::string &job_id)
  {
    // 从数据库获取任务
    auto job_opt = job_storage_->getJob(job_id);
    if (!job_opt)
    {
      spdlog::error("Job not found: {}", job_id);
      return false;
    }

    // 如果任务正在执行，发送取消消息
    if (job_opt->type == JobType::ONCE)
    {
      KafkaMessage message(MessageType::JOB_CANCEL, job_id);
      kafka_client_->sendMessage("job-cancel", message);
    }

    // 从队列中移除任务
    job_queue_->remove(job_id);

    // 更新任务状态
    JobInfo job = *job_opt;
    job_storage_->updateJob(job);

    spdlog::info("Job cancelled: {}", job_id);
    return true;
  }

  JobStatus JobScheduler::get_job_status(const std::string &job_id)
  {
    // 先查询最近的执行记录
    auto executions = job_storage_->getJobExecutions(job_id, 0, 1);
    if (!executions.empty())
    {
      return executions[0].status;
    }

    // 如果没有执行记录，查询任务信息
    auto job_opt = job_storage_->getJob(job_id);
    if (!job_opt)
    {
      spdlog::error("Job not found: {}", job_id);
      return JobStatus::WAITING;
    }

    return JobStatus::WAITING;
  }

  JobResult JobScheduler::get_job_result(const std::string &job_id)
  {
    // 查询最近的执行记录
    auto executions = job_storage_->getJobExecutions(job_id, 0, 1);
    if (executions.empty())
    {
      spdlog::error("No execution found for job: {}", job_id);
      JobResult empty_result;
      empty_result.job_id = job_id;
      empty_result.status = JobStatus::WAITING;
      return empty_result;
    }

    return executions[0];
  }

  void JobScheduler::schedule_loop()
  {
    spdlog::info("Schedule loop started");

    while (running_)
    {
      std::unique_lock<std::mutex> lock(mutex_);

      // 等待新任务或者定时检查
      cv_.wait_for(lock, std::chrono::seconds(5), [this]
                   { return !running_ || job_queue_->size() > 0; });

      if (!running_)
      {
        break;
      }

      // 从数据库获取待处理的任务
      try
      {
        auto pending_jobs = job_storage_->getPendingJobs(10);
        for (const auto &job : pending_jobs)
        {
          job_queue_->push(job);
        }
      }
      catch (const std::exception &e)
      {
        spdlog::error("Failed to get pending jobs: {}", e.what());
      }

      // 处理队列中的任务
      while (running_)
      {
        auto job_opt = job_queue_->pop();
        if (!job_opt)
        {
          break;
        }

        // 检查任务是否需要执行
        if (should_execute(*job_opt))
        {
          // 解锁互斥锁，避免长时间持有
          lock.unlock();

          // 分发任务到执行器
          dispatch_job(*job_opt);

          // 重新获取锁
          lock.lock();
        }
      }
    }

    spdlog::info("Schedule loop stopped");
  }

  bool JobScheduler::should_execute(const JobInfo &job)
  {
    // 对于一次性任务，直接执行
    if (job.type == JobType::ONCE)
    {
      return true;
    }

    // 对于周期性任务，检查是否到达执行时间
    if (job.type == JobType::PERIODIC && !job.cron_expression.empty())
    {
      try
      {
        // 使用Cron表达式解析器检查当前时间是否匹配
        CronParser parser(job.cron_expression);
        if (!parser.matches())
        {
          // 当前时间不匹配Cron表达式，不需要执行
          return false;
        }

        // 检查是否已经执行过
        // 获取最近一次执行记录
        auto executions = job_storage_->getJobExecutions(job.job_id, 0, 1);
        if (!executions.empty())
        {
          // 获取下一次执行时间
          auto nextTime = parser.getNext(executions[0].end_time);
          // 如果当前时间还没到下一次执行时间，不需要执行
          if (std::chrono::system_clock::now() < nextTime)
          {
            return false;
          }
        }
      }
      catch (const std::exception &e)
      {
        spdlog::error("解析Cron表达式失败: {}, 错误: {}", job.cron_expression, e.what());
        return false;
      }
    }

    // 检查是否有可用执行器
    auto executor_opt = executor_registry_->getAvailableExecutor();
    if (!executor_opt)
    {
      spdlog::warn("No available executor for job: {}", job.job_id);
      return false;
    }

    return true;
  }

  void JobScheduler::dispatch_job(const JobInfo &job)
  {
    // 获取可用执行器
    auto executor_opt = executor_registry_->getAvailableExecutor();
    if (!executor_opt)
    {
      spdlog::error("No available executor for job: {}", job.job_id);
      return;
    }

    // 记录执行信息
    job_storage_->saveExecution(job.job_id, executor_opt->first);

    // 发送任务到执行器
    kafka_client_->sendJob("job-submit", job);

    spdlog::info("Job dispatched: {} to executor: {}", job.job_id, executor_opt->first);
  }

  void JobScheduler::handle_result(const JobResult &result)
  {
    // 查询执行记录
    auto executions = job_storage_->getJobExecutions(result.job_id, 0, 1);
    if (executions.empty())
    {
      spdlog::error("No execution found for job: {}", result.job_id);
      return;
    }

    // 更新执行结果
    uint64_t execution_id = executions[0].execution_id;
    if (execution_id == 0)
    {
      spdlog::error("Invalid execution_id (0) for job: {}", result.job_id);
      return;
    }

    job_storage_->updateExecutionResult(execution_id, result.status, result.output, result.error);

    // 更新任务状态
    auto job_opt = job_storage_->getJob(result.job_id);
    if (!job_opt)
    {
      spdlog::error("Job not found: {}", result.job_id);
      return;
    }

    JobInfo job = *job_opt;
    job_storage_->updateJob(job);

    spdlog::info("Job completed: {}, status: {}", result.job_id, static_cast<int>(result.status));
  }

} // namespace scheduler