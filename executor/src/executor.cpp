#include "executor.h"
#include "kafka_message_queue.h"
#include "job_dao.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdlib>
#include <array>
#include <fstream>
#include <iostream>
#include <thread>
#include "config_manager.h"
#include "stats_manager.h"

namespace scheduler
{

  JobExecutor::JobExecutor(const std::string &executor_id)
      : executor_id_(executor_id), running_(false)
  {
    // 初始化数据库连接池
    auto &dbPool = DBConnectionPool::getInstance();
    dbPool.initialize();

    // 创建Kafka客户端
    kafka_client_ = std::make_unique<KafkaMessageQueue>();

    // 初始化Kafka
    std::string kafkaBrokers = ConfigManager::getInstance().getKafkaBrokers();
    kafka_client_->initProducer(kafkaBrokers);
    kafka_client_->initConsumer(kafkaBrokers, "executor-" + executor_id_, {"job-submit", "job-cancel"}, [this](const KafkaMessage &message)
                                {
          if (message.type == MessageType::JOB_SUBMIT)
          {
            try
            {
              // 解析任务信息
              nlohmann::json j = nlohmann::json::parse(message.payload);
              JobInfo job = JobInfo::from_json(j);

              // 添加到任务队列
              std::lock_guard<std::mutex> lock(mutex_);
              job_queue_.push(job);
              cv_.notify_one();

              spdlog::info("接收到任务: {}", job.job_id);
            }
            catch (const std::exception &e)
            {
              spdlog::error("解析任务失败: {}", e.what());
            }
          }
          else if (message.type == MessageType::JOB_CANCEL)
          {
            // 取消任务
            std::string job_id = message.payload;
            spdlog::info("接收到取消任务请求: {}", job_id);
            cancel_job(job_id);
          } });

    spdlog::info("执行器初始化完成: {}", executor_id_);
  }

  JobExecutor::~JobExecutor()
  {
    stop();
  }

  void JobExecutor::start()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_)
    {
      return;
    }

    running_ = true;

    // 向调度中心注册
    register_executor();

    // 启动执行线程
    execute_thread_ = std::thread(&JobExecutor::execute_loop, this);

    // 启动心跳线程
    heartbeat_thread_ = std::thread(&JobExecutor::heartbeat_loop, this);

    // 启动Kafka消费
    kafka_client_->startConsume();

    spdlog::info("执行器已启动: {}", executor_id_);
  }

  void JobExecutor::stop()
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

    // 等待线程结束
    if (execute_thread_.joinable())
    {
      execute_thread_.join();
    }

    if (heartbeat_thread_.joinable())
    {
      heartbeat_thread_.join();
    }

    // 停止Kafka消费
    kafka_client_->stopConsume();

    // 向调度中心注销
    unregister_executor();

    spdlog::info("执行器已停止: {}", executor_id_);
  }

  void JobExecutor::execute_loop()
  {
    spdlog::info("执行线程启动");

    while (running_)
    {
      JobInfo job;

      // 获取任务
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(1), [this]
                     { return !running_ || !job_queue_.empty(); });

        if (!running_)
        {
          break;
        }

        if (job_queue_.empty())
        {
          continue;
        }

        job = job_queue_.front();
        job_queue_.pop();
      }

      // 检查任务是否被取消
      if (is_job_cancelled(job.job_id))
      {
        spdlog::info("任务已被取消，跳过执行: {}", job.job_id);

        // 发送取消结果
        JobResult result;
        result.job_id = job.job_id;
        result.status = JobStatus::FAILED;
        result.error = "任务被取消";
        result.start_time = std::chrono::system_clock::now();
        result.end_time = std::chrono::system_clock::now();

        kafka_client_->sendJobResult("job-result", result);
        continue;
      }

      // 执行任务
      spdlog::info("开始执行任务: {}", job.job_id);
      JobResult result = execute_job(job);
      spdlog::info("任务执行完成: {}, 状态: {}", job.job_id, static_cast<int>(result.status));

      // 发送结果
      kafka_client_->sendJobResult("job-result", result);
    }

    spdlog::info("执行线程退出");
  }

  JobResult JobExecutor::execute_job(const JobInfo &job)
  {
    JobResult result;
    result.job_id = job.job_id;
    result.start_time = std::chrono::system_clock::now();

    // 定义输出和错误变量
    std::string output;
    std::string error;

    // 更新统计信息
    StatsManager::getInstance().updateJobStats(job, JobStatus::RUNNING);

    try
    {
      // 创建临时文件存储命令
      std::string script_file = "/tmp/job_" + job.job_id + ".sh";
      std::ofstream script(script_file);
      script << "#!/bin/bash\n";
      script << job.command << "\n";
      script.close();

      // 设置执行权限
      std::string chmod_cmd = "chmod +x " + script_file;
      std::system(chmod_cmd.c_str());

      // 执行命令
      std::string cmd = script_file + " 2>&1";
      std::array<char, 128> buffer;

      // 设置超时
      int timeout = job.timeout > 0 ? job.timeout : 60; // 默认60秒
      auto start_time = std::chrono::system_clock::now();

      // 创建管道
      FILE *pipe = popen(cmd.c_str(), "r");
      if (!pipe)
      {
        throw std::runtime_error("Failed to create pipe");
      }

      // 读取输出
      while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
      {
        output += buffer.data();

        // 检查是否超时
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        if (elapsed > timeout)
        {
          pclose(pipe);
          throw std::runtime_error("Execution timeout");
        }

        // 检查任务是否被取消
        if (is_job_cancelled(job.job_id))
        {
          pclose(pipe);
          throw std::runtime_error("Job cancelled during execution");
        }
      }

      // 获取返回值
      int status = pclose(pipe);
      if (status == 0)
      {
        result.status = JobStatus::SUCCESS;
      }
      else
      {
        result.status = JobStatus::FAILED;
        error = "Command exited with status " + std::to_string(status);
      }

      // 删除临时文件
      std::remove(script_file.c_str());
    }
    catch (const std::exception &e)
    {
      result.status = JobStatus::FAILED;
      error = e.what();
    }

    result.output = output;
    result.error = error;
    result.end_time = std::chrono::system_clock::now();

    // 更新任务结果统计
    StatsManager::getInstance().updateJobResultStats(result);

    return result;
  }

  void JobExecutor::register_executor()
  {
    // 从配置获取默认最大负载
    int maxLoad = ConfigManager::getInstance().getInt("executor.default_max_load", 10);

    // 向数据库注册执行器
    JobDAO dao;
    dao.registerExecutor(executor_id_, "localhost", 0, maxLoad);
    spdlog::info("执行器已注册: {}, 最大负载: {}", executor_id_, maxLoad);
  }

  void JobExecutor::unregister_executor()
  {
    // 向数据库注销执行器
    JobDAO dao;
    dao.updateExecutorStatus(executor_id_, false);
    spdlog::info("执行器已注销: {}", executor_id_);
  }

  void JobExecutor::heartbeat_loop()
  {
    spdlog::info("心跳线程启动");

    // 从配置获取心跳间隔
    int heartbeatInterval = ConfigManager::getInstance().getInt("executor.heartbeat_interval", 30);
    spdlog::info("心跳间隔设置为 {} 秒", heartbeatInterval);

    while (running_)
    {
      try
      {
        // 更新心跳时间
        JobDAO dao;
        dao.updateExecutorHeartbeat(executor_id_);

        // 发送心跳消息
        KafkaMessage message(MessageType::EXECUTOR_HEARTBEAT, executor_id_);
        kafka_client_->sendMessage("executor-heartbeat", message);

        // 等待下一次心跳
        std::this_thread::sleep_for(std::chrono::seconds(heartbeatInterval));
      }
      catch (const std::exception &e)
      {
        spdlog::error("心跳异常: {}", e.what());
        std::this_thread::sleep_for(std::chrono::seconds(5));
      }
    }

    spdlog::info("心跳线程退出");
  }

  void JobExecutor::cancel_job(const std::string &job_id)
  {
    // 将任务添加到取消列表
    {
      std::lock_guard<std::mutex> lock(cancel_mutex_);
      cancelled_jobs_.insert(job_id);
    }

    // 尝试从队列中移除任务
    {
      std::lock_guard<std::mutex> lock(mutex_);

      // 创建临时队列
      std::queue<JobInfo> temp_queue;
      bool found = false;

      // 遍历当前队列
      while (!job_queue_.empty())
      {
        JobInfo job = job_queue_.front();
        job_queue_.pop();

        // 如果不是要取消的任务，则保留
        if (job.job_id != job_id)
        {
          temp_queue.push(job);
        }
        else
        {
          found = true;
          spdlog::info("从队列中移除已取消的任务: {}", job_id);
        }
      }

      // 恢复队列
      job_queue_ = std::move(temp_queue);

      if (found)
      {
        // 发送取消结果
        JobResult result;
        result.job_id = job_id;
        result.status = JobStatus::FAILED;
        result.error = "任务被取消";
        result.start_time = std::chrono::system_clock::now();
        result.end_time = std::chrono::system_clock::now();

        // 更新统计信息
        StatsManager::getInstance().jobStats_.cancelled_jobs++;

        kafka_client_->sendJobResult("job-result", result);
      }
      else
      {
        spdlog::info("任务不在队列中，可能正在执行或已完成: {}", job_id);
      }
    }
  }

  bool JobExecutor::is_job_cancelled(const std::string &job_id)
  {
    std::lock_guard<std::mutex> lock(cancel_mutex_);
    return cancelled_jobs_.find(job_id) != cancelled_jobs_.end();
  }

} // namespace scheduler