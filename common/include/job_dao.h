#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "job.h"
#include "db_connection_pool.h"

namespace scheduler
{

  // 执行器信息结构体
  struct ExecutorInfo
  {
    std::string executor_id;
    std::string address;
    int current_load;
    int max_load;
    uint64_t total_tasks_executed;
    std::chrono::system_clock::time_point last_heartbeat;
  };

  class JobDAO
  {
  public:
    JobDAO();
    ~JobDAO() = default;

    // 任务信息相关操作
    bool saveJob(const JobInfo &job);
    bool updateJob(const JobInfo &job);
    bool deleteJob(const std::string &jobId);
    std::optional<JobInfo> getJob(const std::string &jobId);
    std::vector<JobInfo> getAllJobs(int offset = 0, int limit = 100);
    std::vector<JobInfo> getPendingJobs(int limit = 100);
    std::vector<JobInfo> getJobsByType(JobType type, int offset = 0, int limit = 100);
    int getJobCount();

    // 任务执行记录相关操作
    bool saveExecution(const std::string &jobId, const std::string &executorId = "");
    bool updateExecutionStatus(uint64_t executionId, JobStatus status);
    bool updateExecutionResult(uint64_t executionId, JobStatus status,
                               const std::string &output, const std::string &error);
    bool updateExecutionTimes(uint64_t executionId,
                              const std::chrono::system_clock::time_point &startTime,
                              const std::chrono::system_clock::time_point &endTime);
    std::vector<JobResult> getJobExecutions(const std::string &jobId, int offset = 0, int limit = 10);
    std::optional<JobResult> getExecution(uint64_t executionId);
    std::vector<JobResult> getRecentExecutions(int limit = 100);
    int getExecutionCount(const std::string &jobId);

    // 执行器节点相关操作
    bool registerExecutor(const std::string &executorId, const std::string &host, int port, int maxLoad = 10);
    bool updateExecutorStatus(const std::string &executorId, bool online);
    bool updateExecutorHeartbeat(const std::string &executorId);
    std::vector<std::pair<std::string, std::string>> getOnlineExecutors();

    // 新增：获取详细的执行器信息列表
    std::vector<ExecutorInfo> getOnlineExecutorsWithLoad();

    // 新增：更新执行器负载信息
    bool incrementExecutorLoad(const std::string &executorId);
    bool decrementExecutorLoad(const std::string &executorId);
    bool updateExecutorMaxLoad(const std::string &executorId, int maxLoad);
    bool incrementExecutorTaskCount(const std::string &executorId);

    // 新增：获取单个执行器信息
    std::optional<ExecutorInfo> getExecutorInfo(const std::string &executorId);

    // 分布式锁相关操作
    bool acquireLock(const std::string &lockName, const std::string &owner, int expireSeconds);
    bool releaseLock(const std::string &lockName, const std::string &owner);
    bool refreshLock(const std::string &lockName, const std::string &owner, int expireSeconds);

    // 系统配置相关操作
    std::string getConfigValue(const std::string &key, const std::string &defaultValue = "");
    bool setConfigValue(const std::string &key, const std::string &value, const std::string &description = "");

    // 清理过期数据
    int cleanupExpiredExecutions(int days);

  private:
    // 时间点转换为MySQL时间戳字符串
    std::string timePointToString(const std::chrono::system_clock::time_point &timePoint);

    // MySQL时间戳字符串转换为时间点
    std::chrono::system_clock::time_point stringToTimePoint(const std::string &timeStr);

    // 从结果集构建JobInfo对象
    JobInfo buildJobInfoFromResult(MYSQL_RES *result);

    // 从结果集构建JobResult对象
    JobResult buildJobResultFromResult(MYSQL_RES *result);

    // 从结果集构建ExecutorInfo对象
    ExecutorInfo buildExecutorInfoFromResult(MYSQL_RES *result);
  };

} // namespace scheduler