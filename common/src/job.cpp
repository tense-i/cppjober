#include "job.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace scheduler
{

  // JobType 转换为字符串
  std::string job_type_to_string(JobType type)
  {
    switch (type)
    {
    case JobType::ONCE:
      return "ONCE";
    case JobType::PERIODIC:
      return "PERIODIC";
    default:
      return "UNKNOWN";
    }
  }

  // 字符串转换为 JobType
  JobType string_to_job_type(const std::string &type_str)
  {
    if (type_str == "ONCE")
    {
      return JobType::ONCE;
    }
    else if (type_str == "PERIODIC")
    {
      return JobType::PERIODIC;
    }
    else
    {
      return JobType::ONCE; // 默认为一次性任务
    }
  }

  // JobStatus 转换为字符串
  std::string job_status_to_string(JobStatus status)
  {
    switch (status)
    {
    case JobStatus::WAITING:
      return "WAITING";
    case JobStatus::RUNNING:
      return "RUNNING";
    case JobStatus::SUCCESS:
      return "SUCCESS";
    case JobStatus::FAILED:
      return "FAILED";
    case JobStatus::TIMEOUT:
      return "TIMEOUT";
    default:
      return "UNKNOWN";
    }
  }

  // 字符串转换为 JobStatus
  JobStatus string_to_job_status(const std::string &status_str)
  {
    if (status_str == "WAITING")
    {
      return JobStatus::WAITING;
    }
    else if (status_str == "RUNNING")
    {
      return JobStatus::RUNNING;
    }
    else if (status_str == "SUCCESS")
    {
      return JobStatus::SUCCESS;
    }
    else if (status_str == "FAILED")
    {
      return JobStatus::FAILED;
    }
    else if (status_str == "TIMEOUT")
    {
      return JobStatus::TIMEOUT;
    }
    else
    {
      return JobStatus::WAITING; // 默认为等待状态
    }
  }

  // 时间点转换为ISO8601字符串
  std::string time_point_to_string(const std::chrono::system_clock::time_point &tp)
  {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%FT%TZ");
    return ss.str();
  }

  // ISO8601字符串转换为时间点
  std::chrono::system_clock::time_point string_to_time_point(const std::string &time_str)
  {
    std::tm tm = {};
    std::stringstream ss(time_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
  }

  // JobInfo 序列化为 JSON
  nlohmann::json JobInfo::to_json() const
  {
    nlohmann::json j;
    j["job_id"] = job_id;
    j["name"] = name;
    j["command"] = command;
    j["type"] = job_type_to_string(type);
    j["priority"] = priority;
    j["cron_expression"] = cron_expression;
    j["timeout"] = timeout;
    j["retry_count"] = retry_count;
    j["retry_interval"] = retry_interval;
    return j;
  }

  // 从 JSON 反序列化为 JobInfo
  JobInfo JobInfo::from_json(const nlohmann::json &j)
  {
    JobInfo job;
    job.job_id = j.value("job_id", "");
    job.name = j.value("name", "");
    job.command = j.value("command", "");
    job.type = string_to_job_type(j.value("type", "ONCE"));
    job.priority = j.value("priority", 0);
    job.cron_expression = j.value("cron_expression", "");
    job.timeout = j.value("timeout", 0);
    job.retry_count = j.value("retry_count", 0);
    job.retry_interval = j.value("retry_interval", 0);
    return job;
  }

  // JobResult 序列化为 JSON
  nlohmann::json JobResult::to_json() const
  {
    nlohmann::json j;
    j["job_id"] = job_id;
    j["execution_id"] = execution_id;
    j["status"] = job_status_to_string(status);
    j["output"] = output;
    j["error"] = error;
    j["start_time"] = time_point_to_string(start_time);
    j["end_time"] = time_point_to_string(end_time);
    return j;
  }

  // 从 JSON 反序列化为 JobResult
  JobResult JobResult::from_json(const nlohmann::json &j)
  {
    JobResult result;
    result.job_id = j.value("job_id", "");
    result.execution_id = j.value("execution_id", 0ULL);
    result.status = string_to_job_status(j.value("status", "WAITING"));
    result.output = j.value("output", "");
    result.error = j.value("error", "");
    result.start_time = string_to_time_point(j.value("start_time", "1970-01-01T00:00:00Z"));
    result.end_time = string_to_time_point(j.value("end_time", "1970-01-01T00:00:00Z"));
    return result;
  }

} // namespace scheduler