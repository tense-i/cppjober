#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace scheduler
{

  enum class JobType
  {
    ONCE,    // 一次性任务
    PERIODIC // 周期性任务
  };

  enum class JobStatus
  {
    WAITING, // 等待执行
    RUNNING, // 正在执行
    SUCCESS, // 执行成功
    FAILED,  // 执行失败
    TIMEOUT  // 执行超时
  };

  struct JobInfo
  {
    std::string job_id;          // 任务ID
    std::string name;            // 任务名称
    std::string command;         // 执行命令
    JobType type;                // 任务类型
    int priority;                // 任务优先级
    std::string cron_expression; // cron表达式（周期任务）
    int timeout;                 // 超时时间（秒）
    int retry_count;             // 重试次数
    int retry_interval;          // 重试间隔（秒）

    // 序列化为JSON
    nlohmann::json to_json() const;
    // 从JSON反序列化
    static JobInfo from_json(const nlohmann::json &j);
  };

  struct JobResult
  {
    std::string job_id;
    uint64_t execution_id = 0; // 执行ID
    JobStatus status;
    std::string output; // 执行输出
    std::string error;  // 错误信息
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;

    nlohmann::json to_json() const;
    static JobResult from_json(const nlohmann::json &j);
  };

} // namespace scheduler