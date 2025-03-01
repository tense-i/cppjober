#include "stats_manager.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace scheduler
{

  StatsManager::StatsManager()
      : startTime_(std::chrono::system_clock::now())
  {
    // 初始化统计信息
    jobStats_.reset();
    systemStats_.reset();
  }

  StatsManager &StatsManager::getInstance()
  {
    static StatsManager instance;
    return instance;
  }

  void StatsManager::updateJobStats(const JobInfo &job, JobStatus status)
  {
    // 更新任务总数
    jobStats_.total_jobs++;

    // 更新任务类型统计
    if (job.type == JobType::ONCE)
    {
      jobStats_.once_jobs++;
    }
    else if (job.type == JobType::PERIODIC)
    {
      jobStats_.periodic_jobs++;
    }

    // 更新任务状态统计
    switch (status)
    {
    case JobStatus::WAITING:
      jobStats_.pending_jobs++;
      break;
    case JobStatus::RUNNING:
      jobStats_.running_jobs++;
      break;
    case JobStatus::SUCCESS:
      jobStats_.completed_jobs++;
      break;
    case JobStatus::FAILED:
      jobStats_.failed_jobs++;
      break;
    case JobStatus::TIMEOUT:
      jobStats_.timeout_jobs++;
      break;
    }

    spdlog::debug("任务统计已更新: 总数={}, 等待={}, 运行={}, 完成={}, 失败={}, 超时={}",
                  jobStats_.total_jobs.load(), jobStats_.pending_jobs.load(),
                  jobStats_.running_jobs.load(), jobStats_.completed_jobs.load(),
                  jobStats_.failed_jobs.load(), jobStats_.timeout_jobs.load());
  }

  void StatsManager::updateJobResultStats(const JobResult &result)
  {
    // 更新任务状态统计
    switch (result.status)
    {
    case JobStatus::SUCCESS:
      jobStats_.completed_jobs++;
      jobStats_.running_jobs--;
      break;
    case JobStatus::FAILED:
      jobStats_.failed_jobs++;
      jobStats_.running_jobs--;
      break;
    case JobStatus::TIMEOUT:
      jobStats_.timeout_jobs++;
      jobStats_.running_jobs--;
      break;
    }

    // 计算执行时间（毫秒）
    auto executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                             result.end_time - result.start_time)
                             .count();

    // 更新执行时间统计
    jobStats_.total_execution_time += executionTime;

    // 更新最小执行时间
    uint64_t currentMin = jobStats_.min_execution_time.load();
    while (executionTime < currentMin &&
           !jobStats_.min_execution_time.compare_exchange_weak(currentMin, executionTime))
    {
      // 自旋直到成功更新或者不再需要更新
    }

    // 更新最大执行时间
    uint64_t currentMax = jobStats_.max_execution_time.load();
    while (executionTime > currentMax &&
           !jobStats_.max_execution_time.compare_exchange_weak(currentMax, executionTime))
    {
      // 自旋直到成功更新或者不再需要更新
    }

    spdlog::debug("任务执行结果统计已更新: 任务ID={}, 状态={}, 执行时间={}ms",
                  result.job_id, static_cast<int>(result.status), executionTime);
  }

  void StatsManager::updateExecutorStats(const ExecutorInfo &executorInfo)
  {
    std::lock_guard<std::mutex> lock(executorStatsMutex_);

    // 更新或添加执行器统计信息
    ExecutorStats &stats = executorStats_[executorInfo.executor_id];
    stats.executor_id = executorInfo.executor_id;
    stats.address = executorInfo.address;
    stats.current_load = executorInfo.current_load;
    stats.max_load = executorInfo.max_load;
    stats.total_tasks_executed = executorInfo.total_tasks_executed;
    stats.last_heartbeat = executorInfo.last_heartbeat;
    stats.is_online = true;

    spdlog::debug("执行器统计已更新: ID={}, 地址={}, 负载={}/{}, 已执行任务={}",
                  executorInfo.executor_id, executorInfo.address,
                  executorInfo.current_load, executorInfo.max_load,
                  executorInfo.total_tasks_executed);
  }

  void StatsManager::updateSystemStats(const std::string &metric, uint64_t value)
  {
    // 更新系统性能指标
    if (metric == "scheduler_uptime")
    {
      systemStats_.scheduler_uptime = value;
    }
    else if (metric == "db_query_count")
    {
      systemStats_.db_query_count = value;
    }
    else if (metric == "db_query_time")
    {
      systemStats_.db_query_time = value;
    }
    else if (metric == "kafka_msg_sent")
    {
      systemStats_.kafka_msg_sent = value;
    }
    else if (metric == "kafka_msg_received")
    {
      systemStats_.kafka_msg_received = value;
    }
    else if (metric == "scheduler_cycles")
    {
      systemStats_.scheduler_cycles = value;
    }
  }

  void StatsManager::addDbQuery(uint64_t queryTimeMs)
  {
    systemStats_.db_query_count++;
    systemStats_.db_query_time += queryTimeMs;
  }

  void StatsManager::addKafkaMessage(bool sent)
  {
    if (sent)
    {
      systemStats_.kafka_msg_sent++;
    }
    else
    {
      systemStats_.kafka_msg_received++;
    }
  }

  void StatsManager::addSchedulerCycle()
  {
    systemStats_.scheduler_cycles++;

    // 更新调度器运行时间
    auto now = std::chrono::system_clock::now();
    systemStats_.scheduler_uptime = std::chrono::duration_cast<std::chrono::seconds>(
                                        now - startTime_)
                                        .count();
  }

  void StatsManager::incrementCancelledJobs()
  {
    jobStats_.cancelled_jobs++;
  }

  JobStats StatsManager::getJobStats() const
  {
    JobStats stats;
    stats.total_jobs = jobStats_.total_jobs.load();
    stats.pending_jobs = jobStats_.pending_jobs.load();
    stats.running_jobs = jobStats_.running_jobs.load();
    stats.completed_jobs = jobStats_.completed_jobs.load();
    stats.failed_jobs = jobStats_.failed_jobs.load();
    stats.timeout_jobs = jobStats_.timeout_jobs.load();
    stats.cancelled_jobs = jobStats_.cancelled_jobs.load();
    stats.once_jobs = jobStats_.once_jobs.load();
    stats.periodic_jobs = jobStats_.periodic_jobs.load();
    stats.total_execution_time = jobStats_.total_execution_time.load();
    stats.min_execution_time = jobStats_.min_execution_time.load();
    stats.max_execution_time = jobStats_.max_execution_time.load();
    stats.retry_count = jobStats_.retry_count.load();
    return stats;
  }

  std::vector<ExecutorStats> StatsManager::getExecutorStats() const
  {
    std::lock_guard<std::mutex> lock(executorStatsMutex_);

    std::vector<ExecutorStats> result;
    result.reserve(executorStats_.size());

    for (const auto &pair : executorStats_)
    {
      result.push_back(pair.second);
    }

    return result;
  }

  SystemStats StatsManager::getSystemStats() const
  {
    SystemStats stats;
    stats.scheduler_uptime = systemStats_.scheduler_uptime.load();
    stats.db_query_count = systemStats_.db_query_count.load();
    stats.db_query_time = systemStats_.db_query_time.load();
    stats.kafka_msg_sent = systemStats_.kafka_msg_sent.load();
    stats.kafka_msg_received = systemStats_.kafka_msg_received.load();
    stats.scheduler_cycles = systemStats_.scheduler_cycles.load();
    return stats;
  }

  void StatsManager::resetAllStats()
  {
    // 重置任务统计
    jobStats_.reset();

    // 重置执行器统计
    {
      std::lock_guard<std::mutex> lock(executorStatsMutex_);
      executorStats_.clear();
    }

    // 重置系统性能统计
    systemStats_.reset();

    // 重置启动时间
    startTime_ = std::chrono::system_clock::now();

    spdlog::info("所有统计信息已重置");
  }

  std::string StatsManager::generateStatsReport() const
  {
    std::stringstream ss;

    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();

    // 格式化当前时间
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << "===== 任务调度系统统计报告 =====" << std::endl;
    ss << "生成时间: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
    ss << "系统运行时间: " << uptime << " 秒" << std::endl;
    ss << std::endl;

    // 任务统计
    ss << "----- 任务统计 -----" << std::endl;
    ss << "总任务数: " << jobStats_.total_jobs.load() << std::endl;
    ss << "等待中任务: " << jobStats_.pending_jobs.load() << std::endl;
    ss << "运行中任务: " << jobStats_.running_jobs.load() << std::endl;
    ss << "已完成任务: " << jobStats_.completed_jobs.load() << std::endl;
    ss << "失败任务: " << jobStats_.failed_jobs.load() << std::endl;
    ss << "超时任务: " << jobStats_.timeout_jobs.load() << std::endl;
    ss << "取消任务: " << jobStats_.cancelled_jobs.load() << std::endl;
    ss << "一次性任务: " << jobStats_.once_jobs.load() << std::endl;
    ss << "周期性任务: " << jobStats_.periodic_jobs.load() << std::endl;

    // 执行时间统计
    if (jobStats_.completed_jobs.load() > 0)
    {
      ss << "平均执行时间: " << jobStats_.getAvgExecutionTime() << " 毫秒" << std::endl;
      ss << "最小执行时间: " << jobStats_.min_execution_time.load() << " 毫秒" << std::endl;
      ss << "最大执行时间: " << jobStats_.max_execution_time.load() << " 毫秒" << std::endl;
    }
    ss << "重试次数: " << jobStats_.retry_count.load() << std::endl;
    ss << std::endl;

    // 执行器统计
    ss << "----- 执行器统计 -----" << std::endl;
    {
      std::lock_guard<std::mutex> lock(executorStatsMutex_);
      ss << "执行器数量: " << executorStats_.size() << std::endl;

      for (const auto &pair : executorStats_)
      {
        const auto &stats = pair.second;
        ss << "执行器ID: " << stats.executor_id << std::endl;
        ss << "  地址: " << stats.address << std::endl;
        ss << "  负载: " << stats.current_load << "/" << stats.max_load
           << " (" << std::fixed << std::setprecision(2) << (stats.getLoadRatio() * 100) << "%)" << std::endl;
        ss << "  已执行任务: " << stats.total_tasks_executed << std::endl;

        // 格式化最后心跳时间
        auto heartbeat_t = std::chrono::system_clock::to_time_t(stats.last_heartbeat);
        ss << "  最后心跳: " << std::put_time(std::localtime(&heartbeat_t), "%Y-%m-%d %H:%M:%S") << std::endl;
        ss << "  状态: " << (stats.is_online ? "在线" : "离线") << std::endl;
        ss << std::endl;
      }
    }

    // 系统性能统计
    ss << "----- 系统性能统计 -----" << std::endl;
    ss << "调度器运行时间: " << systemStats_.scheduler_uptime.load() << " 秒" << std::endl;
    ss << "数据库查询次数: " << systemStats_.db_query_count.load() << std::endl;
    if (systemStats_.db_query_count.load() > 0)
    {
      ss << "平均数据库查询时间: " << systemStats_.getAvgDbQueryTime() << " 毫秒" << std::endl;
    }
    ss << "Kafka消息发送数: " << systemStats_.kafka_msg_sent.load() << std::endl;
    ss << "Kafka消息接收数: " << systemStats_.kafka_msg_received.load() << std::endl;
    ss << "调度周期数: " << systemStats_.scheduler_cycles.load() << std::endl;

    return ss.str();
  }

  std::string StatsManager::exportStatsAsJson() const
  {
    nlohmann::json j;

    // 系统信息
    j["system"]["uptime"] = systemStats_.scheduler_uptime.load();
    j["system"]["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();

    // 任务统计
    j["jobs"]["total"] = jobStats_.total_jobs.load();
    j["jobs"]["pending"] = jobStats_.pending_jobs.load();
    j["jobs"]["running"] = jobStats_.running_jobs.load();
    j["jobs"]["completed"] = jobStats_.completed_jobs.load();
    j["jobs"]["failed"] = jobStats_.failed_jobs.load();
    j["jobs"]["timeout"] = jobStats_.timeout_jobs.load();
    j["jobs"]["cancelled"] = jobStats_.cancelled_jobs.load();
    j["jobs"]["once"] = jobStats_.once_jobs.load();
    j["jobs"]["periodic"] = jobStats_.periodic_jobs.load();
    j["jobs"]["avg_execution_time"] = jobStats_.getAvgExecutionTime();
    j["jobs"]["min_execution_time"] = jobStats_.min_execution_time.load();
    j["jobs"]["max_execution_time"] = jobStats_.max_execution_time.load();
    j["jobs"]["retry_count"] = jobStats_.retry_count.load();

    // 执行器统计
    j["executors"] = nlohmann::json::array();
    {
      std::lock_guard<std::mutex> lock(executorStatsMutex_);
      for (const auto &pair : executorStats_)
      {
        const auto &stats = pair.second;
        nlohmann::json executor;
        executor["id"] = stats.executor_id;
        executor["address"] = stats.address;
        executor["current_load"] = stats.current_load;
        executor["max_load"] = stats.max_load;
        executor["load_ratio"] = stats.getLoadRatio();
        executor["tasks_executed"] = stats.total_tasks_executed;
        executor["last_heartbeat"] = std::chrono::duration_cast<std::chrono::seconds>(
                                         stats.last_heartbeat.time_since_epoch())
                                         .count();
        executor["online"] = stats.is_online;
        j["executors"].push_back(executor);
      }
    }

    // 系统性能统计
    j["performance"]["db_query_count"] = systemStats_.db_query_count.load();
    j["performance"]["db_query_avg_time"] = systemStats_.getAvgDbQueryTime();
    j["performance"]["kafka_msg_sent"] = systemStats_.kafka_msg_sent.load();
    j["performance"]["kafka_msg_received"] = systemStats_.kafka_msg_received.load();
    j["performance"]["scheduler_cycles"] = systemStats_.scheduler_cycles.load();

    return j.dump(2); // 缩进2个空格
  }

} // namespace scheduler