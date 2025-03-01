#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include "job.h"
#include "job_dao.h"
#include "scheduler.h"

// 前向声明
namespace httplib
{
  using Params = std::multimap<std::string, std::string>;
}

namespace scheduler
{
  /**
   * @brief 任务管理API处理器
   */
  class JobApiHandler
  {
  public:
    /**
     * @brief 构造函数
     * @param scheduler 调度器实例的引用
     */
    JobApiHandler(JobScheduler &scheduler);

    /**
     * @brief 处理任务API请求
     * @param path 请求路径
     * @param method HTTP方法
     * @param query_params 查询参数
     * @param content 请求内容
     * @return HTTP响应
     */
    std::string handleRequest(const std::string &path,
                              const std::string &method,
                              const httplib::Params &query_params,
                              const std::string &content);

  private:
    // 获取任务列表
    std::string getJobs(const httplib::Params &params);

    // 获取任务详情
    std::string getJob(const std::string &jobId);

    // 添加任务
    std::string addJob(const std::string &content);

    // 更新任务
    std::string updateJob(const std::string &jobId, const std::string &content);

    // 删除任务
    std::string deleteJob(const std::string &jobId);

    // 执行任务
    std::string executeJob(const std::string &jobId);

    // 获取任务执行记录
    std::string getJobExecutions(const std::string &jobId, const httplib::Params &params);

    // 调度器引用
    JobScheduler &scheduler_;
    // 数据访问对象
    std::unique_ptr<JobDAO> jobDao_;
  };

} // namespace scheduler