#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mysql/mysql.h>
#include "job.h"
#include <mutex>

namespace scheduler
{

  class JobStorage
  {
  public:
    JobStorage(const std::string &host,
               const std::string &user,
               const std::string &password,
               const std::string &database);
    ~JobStorage();

    // 保存任务信息
    bool save_job(const JobInfo &job);
    // 更新任务状态
    bool update_job_status(const std::string &job_id, JobStatus status);
    // 保存任务结果
    bool save_job_result(const JobResult &result);
    // 获取任务信息
    JobInfo get_job(const std::string &job_id);
    // 获取任务结果
    JobResult get_result(const std::string &job_id);
    // 获取所有待执行的任务
    std::vector<JobInfo> get_pending_jobs();
    // 获取所有执行中的任务
    std::vector<JobInfo> get_running_jobs();
    // 清理过期的任务记录
    void cleanup_expired_jobs(int days);

  private:
    bool connect();
    void disconnect();

    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    MYSQL *mysql_;
    std::mutex mutex_;
  };

} // namespace scheduler