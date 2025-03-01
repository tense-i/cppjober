#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include "job_dao.h"

// 前向声明
namespace httplib
{
  using Params = std::multimap<std::string, std::string>;
}

namespace scheduler
{
  /**
   * @brief 执行器管理API处理器
   */
  class ExecutorApiHandler
  {
  public:
    /**
     * @brief 构造函数
     */
    ExecutorApiHandler();

    /**
     * @brief 处理执行器API请求
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
    // 获取执行器列表
    std::string getExecutors();

    // 获取执行器详情
    std::string getExecutor(const std::string &executorId);

    // 更新执行器最大负载
    std::string updateExecutorMaxLoad(const std::string &executorId, const std::string &content);

    // 禁用/启用执行器
    std::string updateExecutorStatus(const std::string &executorId, const std::string &content);

    // 获取执行器任务列表
    std::string getExecutorTasks(const std::string &executorId, const httplib::Params &params);

    // 数据访问对象
    std::unique_ptr<JobDAO> jobDao_;
  };

} // namespace scheduler