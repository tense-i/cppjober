#include "job_dao.h"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace scheduler
{

  JobDAO::JobDAO()
  {
    // 构造函数，不需要特殊初始化
  }

  // 时间点转换为MySQL时间戳字符串
  std::string JobDAO::timePointToString(const std::chrono::system_clock::time_point &timePoint)
  {
    auto time_t = std::chrono::system_clock::to_time_t(timePoint);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  timePoint.time_since_epoch())
                  .count() %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms;

    return ss.str();
  }

  // MySQL时间戳字符串转换为时间点
  std::chrono::system_clock::time_point JobDAO::stringToTimePoint(const std::string &timeStr)
  {
    std::tm tm = {};
    int milliseconds = 0;

    // 解析日期时间部分
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    // 解析毫秒部分（如果有）
    if (ss.peek() == '.')
    {
      char dot;
      ss >> dot >> milliseconds;
    }

    auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    timePoint += std::chrono::milliseconds(milliseconds);

    return timePoint;
  }

  // 保存任务信息
  bool JobDAO::saveJob(const JobInfo &job)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::string jobType;
    if (job.type == JobType::ONCE)
    {
      jobType = "ONCE";
    }
    else if (job.type == JobType::PERIODIC)
    {
      jobType = "PERIODIC";
    }

    std::stringstream ss;
    ss << "INSERT INTO job_info (job_id, name, command, job_type, priority, "
       << "cron_expression, timeout, retry_count, retry_interval) VALUES ("
       << "'" << job.job_id << "', "
       << "'" << job.name << "', "
       << "'" << job.command << "', "
       << "'" << jobType << "', "
       << job.priority << ", "
       << (job.cron_expression.empty() ? "NULL" : ("'" + job.cron_expression + "'")) << ", "
       << job.timeout << ", "
       << job.retry_count << ", "
       << job.retry_interval << ")";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to save job: {}", job.job_id);
    }
    else
    {
      spdlog::info("Job saved successfully: {}", job.job_id);
    }

    return result;
  }

  // 更新任务信息
  bool JobDAO::updateJob(const JobInfo &job)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::string jobType;
    if (job.type == JobType::ONCE)
    {
      jobType = "ONCE";
    }
    else if (job.type == JobType::PERIODIC)
    {
      jobType = "PERIODIC";
    }

    std::stringstream ss;
    ss << "UPDATE job_info SET "
       << "name = '" << job.name << "', "
       << "command = '" << job.command << "', "
       << "job_type = '" << jobType << "', "
       << "priority = " << job.priority << ", "
       << "cron_expression = " << (job.cron_expression.empty() ? "NULL" : ("'" + job.cron_expression + "'")) << ", "
       << "timeout = " << job.timeout << ", "
       << "retry_count = " << job.retry_count << ", "
       << "retry_interval = " << job.retry_interval
       << " WHERE job_id = '" << job.job_id << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update job: {}", job.job_id);
    }
    else
    {
      spdlog::info("Job updated successfully: {}", job.job_id);
    }

    return result;
  }

  // 删除任务
  bool JobDAO::deleteJob(const std::string &jobId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "DELETE FROM job_info WHERE job_id = '" << jobId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to delete job: {}", jobId);
    }
    else
    {
      spdlog::info("Job deleted successfully: {}", jobId);
    }

    return result;
  }

  // 从结果集构建JobInfo对象
  JobInfo JobDAO::buildJobInfoFromResult(MYSQL_RES *result)
  {
    JobInfo job;

    if (!result)
    {
      return job;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
      return job;
    }

    unsigned long *lengths = mysql_fetch_lengths(result);

    // 获取字段值
    if (row[0])
      job.job_id = std::string(row[0], lengths[0]);
    if (row[1])
      job.name = std::string(row[1], lengths[1]);
    if (row[2])
      job.command = std::string(row[2], lengths[2]);

    // 解析任务类型
    if (row[3])
    {
      std::string typeStr = std::string(row[3], lengths[3]);
      if (typeStr == "ONCE")
      {
        job.type = JobType::ONCE;
      }
      else if (typeStr == "PERIODIC")
      {
        job.type = JobType::PERIODIC;
      }
    }

    // 解析其他字段
    if (row[4])
      job.priority = std::stoi(std::string(row[4], lengths[4]));
    if (row[5])
      job.cron_expression = std::string(row[5], lengths[5]);
    if (row[6])
      job.timeout = std::stoi(std::string(row[6], lengths[6]));
    if (row[7])
      job.retry_count = std::stoi(std::string(row[7], lengths[7]));
    if (row[8])
      job.retry_interval = std::stoi(std::string(row[8], lengths[8]));

    return job;
  }

  // 获取任务信息
  std::optional<JobInfo> JobDAO::getJob(const std::string &jobId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return std::nullopt;
    }

    std::stringstream ss;
    ss << "SELECT job_id, name, command, job_type, priority, "
       << "cron_expression, timeout, retry_count, retry_interval "
       << "FROM job_info WHERE job_id = '" << jobId << "'";

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query job: {}", jobId);
      return std::nullopt;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result || mysql_num_rows(result) == 0)
    {
      if (result)
        mysql_free_result(result);
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::warn("Job not found: {}", jobId);
      return std::nullopt;
    }

    JobInfo job = buildJobInfoFromResult(result);
    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return job;
  }

  // 获取所有任务
  std::vector<JobInfo> JobDAO::getAllJobs(int offset, int limit)
  {
    std::vector<JobInfo> jobs;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return jobs;
    }

    std::stringstream ss;
    ss << "SELECT job_id, name, command, job_type, priority, "
       << "cron_expression, timeout, retry_count, retry_interval "
       << "FROM job_info ORDER BY priority DESC, create_time DESC "
       << "LIMIT " << limit << " OFFSET " << offset;

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query jobs");
      return jobs;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return jobs;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      mysql_data_seek(result, jobs.size());
      JobInfo job = buildJobInfoFromResult(result);
      jobs.push_back(job);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return jobs;
  }

  // 获取待执行的任务
  std::vector<JobInfo> JobDAO::getPendingJobs(int limit)
  {
    std::vector<JobInfo> jobs;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return jobs;
    }

    // 查询没有正在执行的任务
    std::stringstream ss;
    ss << "SELECT j.job_id, j.name, j.command, j.job_type, j.priority, "
       << "j.cron_expression, j.timeout, j.retry_count, j.retry_interval "
       << "FROM job_info j "
       << "LEFT JOIN job_execution e ON j.job_id = e.job_id AND e.status = 'RUNNING' "
       << "WHERE e.job_id IS NULL "
       << "ORDER BY j.priority DESC, j.create_time ASC "
       << "LIMIT " << limit;

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query pending jobs");
      return jobs;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return jobs;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      mysql_data_seek(result, jobs.size());
      JobInfo job = buildJobInfoFromResult(result);
      jobs.push_back(job);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return jobs;
  }

  // 按类型获取任务
  std::vector<JobInfo> JobDAO::getJobsByType(JobType type, int offset, int limit)
  {
    std::vector<JobInfo> jobs;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return jobs;
    }

    std::string jobType;
    if (type == JobType::ONCE)
    {
      jobType = "ONCE";
    }
    else if (type == JobType::PERIODIC)
    {
      jobType = "PERIODIC";
    }

    std::stringstream ss;
    ss << "SELECT job_id, name, command, job_type, priority, "
       << "cron_expression, timeout, retry_count, retry_interval "
       << "FROM job_info WHERE job_type = '" << jobType << "' "
       << "ORDER BY priority DESC, create_time DESC "
       << "LIMIT " << limit << " OFFSET " << offset;

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query jobs by type");
      return jobs;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return jobs;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      mysql_data_seek(result, jobs.size());
      JobInfo job = buildJobInfoFromResult(result);
      jobs.push_back(job);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return jobs;
  }

  // 获取任务总数
  int JobDAO::getJobCount()
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return 0;
    }

    std::string query = "SELECT COUNT(*) FROM job_info";

    if (!conn->executeQuery(query))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to count jobs");
      return 0;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = 0;

    if (row && row[0])
    {
      count = std::stoi(row[0]);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return count;
  }

  // 从结果集构建JobResult对象
  JobResult JobDAO::buildJobResultFromResult(MYSQL_RES *result)
  {
    JobResult jobResult;

    if (!result)
    {
      return jobResult;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
      return jobResult;
    }

    unsigned long *lengths = mysql_fetch_lengths(result);

    // 获取字段值
    if (row[0]) // execution_id
    {
      try
      {
        jobResult.execution_id = std::stoull(std::string(row[0], lengths[0]));
      }
      catch (const std::exception &e)
      {
        spdlog::error("Failed to parse execution_id: {}", e.what());
      }
    }
    if (row[1])
      jobResult.job_id = std::string(row[1], lengths[1]);

    // 解析任务状态
    if (row[3])
    {
      std::string statusStr = std::string(row[3], lengths[3]);
      if (statusStr == "WAITING")
      {
        jobResult.status = JobStatus::WAITING;
      }
      else if (statusStr == "RUNNING")
      {
        jobResult.status = JobStatus::RUNNING;
      }
      else if (statusStr == "SUCCESS")
      {
        jobResult.status = JobStatus::SUCCESS;
      }
      else if (statusStr == "FAILED")
      {
        jobResult.status = JobStatus::FAILED;
      }
      else if (statusStr == "TIMEOUT")
      {
        jobResult.status = JobStatus::TIMEOUT;
      }
    }

    // 解析开始和结束时间
    if (row[4])
      jobResult.start_time = stringToTimePoint(std::string(row[4], lengths[4]));
    if (row[5])
      jobResult.end_time = stringToTimePoint(std::string(row[5], lengths[5]));

    // 解析输出和错误信息
    if (row[6])
      jobResult.output = std::string(row[6], lengths[6]);
    if (row[7])
      jobResult.error = std::string(row[7], lengths[7]);

    return jobResult;
  }

  // 保存任务执行记录
  bool JobDAO::saveExecution(const std::string &jobId, const std::string &executorId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "INSERT INTO job_execution (job_id, executor_id, status) VALUES ("
       << "'" << jobId << "', "
       << (executorId.empty() ? "NULL" : ("'" + executorId + "'")) << ", "
       << "'WAITING')";

    bool result = conn->executeUpdate(ss.str());
    uint64_t executionId = 0;

    if (result)
    {
      executionId = conn->getLastInsertId();
    }

    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to save execution for job: {}", jobId);
    }
    else
    {
      spdlog::info("Execution saved successfully for job: {}, execution ID: {}", jobId, executionId);
    }

    return result;
  }

  // 更新任务执行状态
  bool JobDAO::updateExecutionStatus(uint64_t executionId, JobStatus status)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::string statusStr;
    switch (status)
    {
    case JobStatus::WAITING:
      statusStr = "WAITING";
      break;
    case JobStatus::RUNNING:
      statusStr = "RUNNING";
      break;
    case JobStatus::SUCCESS:
      statusStr = "SUCCESS";
      break;
    case JobStatus::FAILED:
      statusStr = "FAILED";
      break;
    case JobStatus::TIMEOUT:
      statusStr = "TIMEOUT";
      break;
    }

    std::stringstream ss;
    ss << "UPDATE job_execution SET status = '" << statusStr << "' "
       << "WHERE execution_id = " << executionId;

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update execution status: {}", executionId);
    }
    else
    {
      spdlog::info("Execution status updated successfully: {}, status: {}", executionId, statusStr);
    }

    return result;
  }

  // 更新任务执行结果
  bool JobDAO::updateExecutionResult(uint64_t executionId, JobStatus status,
                                     const std::string &output, const std::string &error)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::string statusStr;
    switch (status)
    {
    case JobStatus::WAITING:
      statusStr = "WAITING";
      break;
    case JobStatus::RUNNING:
      statusStr = "RUNNING";
      break;
    case JobStatus::SUCCESS:
      statusStr = "SUCCESS";
      break;
    case JobStatus::FAILED:
      statusStr = "FAILED";
      break;
    case JobStatus::TIMEOUT:
      statusStr = "TIMEOUT";
      break;
    }

    // 转义输出和错误信息中的特殊字符
    char *escapedOutput = new char[output.length() * 2 + 1];
    char *escapedError = new char[error.length() * 2 + 1];

    mysql_real_escape_string(conn->getRawConnection(), escapedOutput, output.c_str(), output.length());
    mysql_real_escape_string(conn->getRawConnection(), escapedError, error.c_str(), error.length());

    std::stringstream ss;
    ss << "UPDATE job_execution SET "
       << "status = '" << statusStr << "', "
       << "output = '" << escapedOutput << "', "
       << "error = '" << escapedError << "', "
       << "end_time = CURRENT_TIMESTAMP "
       << "WHERE execution_id = " << executionId;

    bool result = conn->executeUpdate(ss.str());

    delete[] escapedOutput;
    delete[] escapedError;

    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update execution result: {}", executionId);
    }
    else
    {
      spdlog::info("Execution result updated successfully: {}", executionId);
    }

    return result;
  }

  // 更新任务执行时间
  bool JobDAO::updateExecutionTimes(uint64_t executionId,
                                    const std::chrono::system_clock::time_point &startTime,
                                    const std::chrono::system_clock::time_point &endTime)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::string startTimeStr = timePointToString(startTime);
    std::string endTimeStr;

    // 如果结束时间是默认值，则设置为NULL
    if (endTime == std::chrono::system_clock::time_point())
    {
      endTimeStr = "NULL";
    }
    else
    {
      endTimeStr = "'" + timePointToString(endTime) + "'";
    }

    std::stringstream ss;
    ss << "UPDATE job_execution SET "
       << "start_time = '" << startTimeStr << "', "
       << "end_time = " << endTimeStr << " "
       << "WHERE execution_id = " << executionId;

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update execution times: {}", executionId);
    }
    else
    {
      spdlog::info("Execution times updated successfully: {}", executionId);
    }

    return result;
  }

  // 获取任务执行记录
  std::vector<JobResult> JobDAO::getJobExecutions(const std::string &jobId, int offset, int limit)
  {
    std::vector<JobResult> results;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return results;
    }

    std::stringstream ss;
    ss << "SELECT execution_id, job_id, executor_id, status, start_time, end_time, output, error "
       << "FROM job_execution WHERE job_id = '" << jobId << "' "
       << "ORDER BY trigger_time DESC "
       << "LIMIT " << limit << " OFFSET " << offset;

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query job executions: {}", jobId);
      return results;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return results;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      mysql_data_seek(result, results.size());
      JobResult jobResult = buildJobResultFromResult(result);
      results.push_back(jobResult);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return results;
  }

  // 获取单个执行记录
  std::optional<JobResult> JobDAO::getExecution(uint64_t executionId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return std::nullopt;
    }

    std::stringstream ss;
    ss << "SELECT execution_id, job_id, executor_id, status, start_time, end_time, output, error "
       << "FROM job_execution WHERE execution_id = " << executionId;

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query execution: {}", executionId);
      return std::nullopt;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result || mysql_num_rows(result) == 0)
    {
      if (result)
        mysql_free_result(result);
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::warn("Execution not found: {}", executionId);
      return std::nullopt;
    }

    JobResult jobResult = buildJobResultFromResult(result);
    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return jobResult;
  }

  // 获取最近的执行记录
  std::vector<JobResult> JobDAO::getRecentExecutions(int limit)
  {
    std::vector<JobResult> results;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return results;
    }

    std::stringstream ss;
    ss << "SELECT execution_id, job_id, executor_id, status, start_time, end_time, output, error "
       << "FROM job_execution "
       << "ORDER BY trigger_time DESC "
       << "LIMIT " << limit;

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query recent executions");
      return results;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return results;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      mysql_data_seek(result, results.size());
      JobResult jobResult = buildJobResultFromResult(result);
      results.push_back(jobResult);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return results;
  }

  // 获取任务执行记录数量
  int JobDAO::getExecutionCount(const std::string &jobId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return 0;
    }

    std::stringstream ss;
    ss << "SELECT COUNT(*) FROM job_execution WHERE job_id = '" << jobId << "'";

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to count executions for job: {}", jobId);
      return 0;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = 0;

    if (row && row[0])
    {
      count = std::stoi(row[0]);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return count;
  }

  // 执行器节点相关操作

  // 注册执行器节点
  bool JobDAO::registerExecutor(const std::string &executorId, const std::string &host, int port, int maxLoad)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "INSERT INTO executor_node (executor_id, host, port, status, max_load) VALUES ("
       << "'" << executorId << "', "
       << "'" << host << "', "
       << port << ", "
       << "'ONLINE', "
       << maxLoad << ") "
       << "ON DUPLICATE KEY UPDATE "
       << "host = '" << host << "', "
       << "port = " << port << ", "
       << "status = 'ONLINE', "
       << "max_load = " << maxLoad << ", "
       << "last_heartbeat = CURRENT_TIMESTAMP";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to register executor: {}", executorId);
    }
    else
    {
      spdlog::info("Executor registered successfully: {}", executorId);
    }

    return result;
  }

  // 更新执行器状态
  bool JobDAO::updateExecutorStatus(const std::string &executorId, bool online)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::string status = online ? "ONLINE" : "OFFLINE";

    std::stringstream ss;
    ss << "UPDATE executor_node SET "
       << "status = '" << status << "', "
       << "last_heartbeat = " << (online ? "CURRENT_TIMESTAMP" : "last_heartbeat")
       << " WHERE executor_id = '" << executorId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update executor status: {}", executorId);
    }
    else
    {
      spdlog::info("Executor status updated successfully: {}, status: {}", executorId, status);
    }

    return result;
  }

  // 更新执行器心跳
  bool JobDAO::updateExecutorHeartbeat(const std::string &executorId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "UPDATE executor_node SET "
       << "last_heartbeat = CURRENT_TIMESTAMP "
       << "WHERE executor_id = '" << executorId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update executor heartbeat: {}", executorId);
    }
    else
    {
      spdlog::debug("Executor heartbeat updated: {}", executorId);
    }

    return result;
  }

  // 获取在线执行器列表
  std::vector<std::pair<std::string, std::string>> JobDAO::getOnlineExecutors()
  {
    std::vector<std::pair<std::string, std::string>> executors;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return executors;
    }

    // 获取最近5分钟有心跳的执行器
    std::stringstream ss;
    ss << "SELECT executor_id, host, port FROM executor_node "
       << "WHERE status = 'ONLINE' AND "
       << "last_heartbeat > DATE_SUB(NOW(), INTERVAL 5 MINUTE)";

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query online executors");
      return executors;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return executors;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      if (row[0] && row[1] && row[2])
      {
        std::string executorId = row[0];
        std::string host = row[1];
        int port = std::stoi(row[2]);

        // 构建地址字符串
        std::stringstream addrSs;
        addrSs << host << ":" << port;

        executors.emplace_back(executorId, addrSs.str());
      }
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return executors;
  }

  // 分布式锁相关操作

  // 获取分布式锁
  bool JobDAO::acquireLock(const std::string &lockName, const std::string &owner, int expireSeconds)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    // 首先清理过期的锁
    std::stringstream cleanSs;
    cleanSs << "DELETE FROM job_lock WHERE expire_time < NOW()";
    conn->executeUpdate(cleanSs.str());

    // 尝试获取锁
    std::stringstream ss;
    ss << "INSERT INTO job_lock (lock_name, lock_owner, expire_time) VALUES ("
       << "'" << lockName << "', "
       << "'" << owner << "', "
       << "DATE_ADD(NOW(), INTERVAL " << expireSeconds << " SECOND)) "
       << "ON DUPLICATE KEY UPDATE "
       << "lock_owner = IF(expire_time < NOW(), VALUES(lock_owner), lock_owner), "
       << "expire_time = IF(expire_time < NOW() OR lock_owner = VALUES(lock_owner), "
       << "VALUES(expire_time), expire_time), "
       << "lock_time = IF(expire_time < NOW() OR lock_owner = VALUES(lock_owner), "
       << "NOW(), lock_time)";

    bool result = conn->executeUpdate(ss.str());

    // 检查是否真的获取了锁
    if (result)
    {
      std::stringstream checkSs;
      checkSs << "SELECT lock_owner FROM job_lock "
              << "WHERE lock_name = '" << lockName << "' "
              << "AND lock_owner = '" << owner << "'";

      if (!conn->executeQuery(checkSs.str()))
      {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
      }

      MYSQL_RES *checkResult = conn->getResult();
      if (!checkResult || mysql_num_rows(checkResult) == 0)
      {
        if (checkResult)
          mysql_free_result(checkResult);
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
      }

      mysql_free_result(checkResult);
    }

    DBConnectionPool::getInstance().releaseConnection(conn);

    if (result)
    {
      spdlog::debug("Lock acquired: {}, owner: {}", lockName, owner);
    }
    else
    {
      spdlog::debug("Failed to acquire lock: {}", lockName);
    }

    return result;
  }

  // 释放分布式锁
  bool JobDAO::releaseLock(const std::string &lockName, const std::string &owner)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "DELETE FROM job_lock "
       << "WHERE lock_name = '" << lockName << "' "
       << "AND lock_owner = '" << owner << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (result)
    {
      spdlog::debug("Lock released: {}, owner: {}", lockName, owner);
    }
    else
    {
      spdlog::debug("Failed to release lock: {}, owner: {}", lockName, owner);
    }

    return result;
  }

  // 刷新分布式锁
  bool JobDAO::refreshLock(const std::string &lockName, const std::string &owner, int expireSeconds)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "UPDATE job_lock SET "
       << "expire_time = DATE_ADD(NOW(), INTERVAL " << expireSeconds << " SECOND) "
       << "WHERE lock_name = '" << lockName << "' "
       << "AND lock_owner = '" << owner << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (result)
    {
      spdlog::debug("Lock refreshed: {}, owner: {}", lockName, owner);
    }
    else
    {
      spdlog::debug("Failed to refresh lock: {}, owner: {}", lockName, owner);
    }

    return result;
  }

  // 系统配置相关操作

  // 获取系统配置值
  std::string JobDAO::getConfigValue(const std::string &key, const std::string &defaultValue)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return defaultValue;
    }

    std::stringstream ss;
    ss << "SELECT config_value FROM system_config "
       << "WHERE config_key = '" << key << "'";

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query config: {}", key);
      return defaultValue;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result || mysql_num_rows(result) == 0)
    {
      if (result)
        mysql_free_result(result);
      DBConnectionPool::getInstance().releaseConnection(conn);
      return defaultValue;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    std::string value = defaultValue;

    if (row && row[0])
    {
      value = row[0];
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return value;
  }

  // 设置系统配置值
  bool JobDAO::setConfigValue(const std::string &key, const std::string &value, const std::string &description)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "INSERT INTO system_config (config_key, config_value, description) VALUES ("
       << "'" << key << "', "
       << "'" << value << "', "
       << (description.empty() ? "NULL" : ("'" + description + "'")) << ") "
       << "ON DUPLICATE KEY UPDATE "
       << "config_value = '" << value << "', "
       << "description = " << (description.empty() ? "description" : ("'" + description + "'"));

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to set config: {}", key);
    }
    else
    {
      spdlog::info("Config set successfully: {}", key);
    }

    return result;
  }

  // 清理过期的执行记录
  int JobDAO::cleanupExpiredExecutions(int days)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return 0;
    }

    std::stringstream ss;
    ss << "DELETE FROM job_execution "
       << "WHERE trigger_time < DATE_SUB(NOW(), INTERVAL " << days << " DAY)";

    if (!conn->executeUpdate(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to cleanup expired executions");
      return 0;
    }

    int count = conn->getAffectedRows();
    DBConnectionPool::getInstance().releaseConnection(conn);

    spdlog::info("Cleaned up {} expired executions", count);
    return count;
  }

  // 从结果集构建ExecutorInfo对象
  ExecutorInfo JobDAO::buildExecutorInfoFromResult(MYSQL_RES *result)
  {
    ExecutorInfo info;

    if (!result)
    {
      return info;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
      return info;
    }

    unsigned long *lengths = mysql_fetch_lengths(result);

    // 获取字段值
    if (row[0])
      info.executor_id = std::string(row[0], lengths[0]);

    // 构建地址
    if (row[1] && row[2])
    {
      std::string host = std::string(row[1], lengths[1]);
      int port = std::stoi(std::string(row[2], lengths[2]));
      std::stringstream addrSs;
      addrSs << host << ":" << port;
      info.address = addrSs.str();
    }

    // 获取负载信息
    if (row[3])
      info.current_load = std::stoi(std::string(row[3], lengths[3]));
    if (row[4])
      info.max_load = std::stoi(std::string(row[4], lengths[4]));
    if (row[5])
      info.total_tasks_executed = std::stoull(std::string(row[5], lengths[5]));
    if (row[6])
      info.last_heartbeat = stringToTimePoint(std::string(row[6], lengths[6]));

    return info;
  }

  // 获取详细的执行器信息列表
  std::vector<ExecutorInfo> JobDAO::getOnlineExecutorsWithLoad()
  {
    std::vector<ExecutorInfo> executors;

    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return executors;
    }

    // 获取最近5分钟有心跳的执行器
    std::stringstream ss;
    ss << "SELECT executor_id, host, port, current_load, max_load, total_tasks_executed, last_heartbeat "
       << "FROM executor_node "
       << "WHERE status = 'ONLINE' AND "
       << "last_heartbeat > DATE_SUB(NOW(), INTERVAL 5 MINUTE)";

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query online executors with load");
      return executors;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result)
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to get result set");
      return executors;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
      mysql_data_seek(result, executors.size());
      ExecutorInfo info = buildExecutorInfoFromResult(result);
      executors.push_back(info);
    }

    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return executors;
  }

  // 获取单个执行器信息
  std::optional<ExecutorInfo> JobDAO::getExecutorInfo(const std::string &executorId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return std::nullopt;
    }

    std::stringstream ss;
    ss << "SELECT executor_id, host, port, current_load, max_load, total_tasks_executed, last_heartbeat "
       << "FROM executor_node "
       << "WHERE executor_id = '" << executorId << "'";

    if (!conn->executeQuery(ss.str()))
    {
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::error("Failed to query executor info: {}", executorId);
      return std::nullopt;
    }

    MYSQL_RES *result = conn->getResult();
    if (!result || mysql_num_rows(result) == 0)
    {
      if (result)
        mysql_free_result(result);
      DBConnectionPool::getInstance().releaseConnection(conn);
      spdlog::warn("Executor not found: {}", executorId);
      return std::nullopt;
    }

    ExecutorInfo info = buildExecutorInfoFromResult(result);
    mysql_free_result(result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return info;
  }

  // 增加执行器负载
  bool JobDAO::incrementExecutorLoad(const std::string &executorId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "UPDATE executor_node SET "
       << "current_load = current_load + 1 "
       << "WHERE executor_id = '" << executorId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to increment executor load: {}", executorId);
    }
    else
    {
      spdlog::debug("Executor load incremented: {}", executorId);
    }

    return result;
  }

  // 减少执行器负载
  bool JobDAO::decrementExecutorLoad(const std::string &executorId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "UPDATE executor_node SET "
       << "current_load = GREATEST(0, current_load - 1) "
       << "WHERE executor_id = '" << executorId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to decrement executor load: {}", executorId);
    }
    else
    {
      spdlog::debug("Executor load decremented: {}", executorId);
    }

    return result;
  }

  // 更新执行器最大负载
  bool JobDAO::updateExecutorMaxLoad(const std::string &executorId, int maxLoad)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "UPDATE executor_node SET "
       << "max_load = " << maxLoad << " "
       << "WHERE executor_id = '" << executorId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to update executor max load: {}", executorId);
    }
    else
    {
      spdlog::info("Executor max load updated: {}, max_load: {}", executorId, maxLoad);
    }

    return result;
  }

  // 增加执行器已执行任务数
  bool JobDAO::incrementExecutorTaskCount(const std::string &executorId)
  {
    auto conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
    {
      spdlog::error("Failed to get database connection");
      return false;
    }

    std::stringstream ss;
    ss << "UPDATE executor_node SET "
       << "total_tasks_executed = total_tasks_executed + 1 "
       << "WHERE executor_id = '" << executorId << "'";

    bool result = conn->executeUpdate(ss.str());
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (!result)
    {
      spdlog::error("Failed to increment executor task count: {}", executorId);
    }
    else
    {
      spdlog::debug("Executor task count incremented: {}", executorId);
    }

    return result;
  }

} // namespace scheduler