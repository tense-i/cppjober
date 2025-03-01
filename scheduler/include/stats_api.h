#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include "stats_manager.h"
#include "job_api.h"
#include "executor_api.h"

// 前向声明
namespace httplib
{
  using Params = std::multimap<std::string, std::string>;
}

namespace scheduler
{
  // 前向声明
  class JobScheduler;

  /**
   * @brief 统计信息API处理器
   */
  class StatsApiHandler
  {
  public:
    /**
     * @brief 处理统计信息API请求
     * @param path 请求路径
     * @param method HTTP方法
     * @param query_params 查询参数
     * @return HTTP响应
     */
    static std::string handleRequest(const std::string &path,
                                     const std::string &method,
                                     const httplib::Params &query_params);

  private:
    // 获取所有统计信息
    static std::string getAllStats();

    // 获取任务统计信息
    static std::string getJobStats();

    // 获取执行器统计信息
    static std::string getExecutorStats();

    // 获取系统性能统计信息
    static std::string getSystemStats();

    // 重置统计信息
    static std::string resetStats();
  };

  /**
   * @brief 简单的HTTP服务器，用于提供API
   */
  class ApiServer
  {
  public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param scheduler 调度器实例的引用
     */
    ApiServer(int port, JobScheduler &scheduler);

    /**
     * @brief 析构函数
     */
    ~ApiServer();

    /**
     * @brief 启动服务器
     */
    void start();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 检查服务器是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;

  private:
    // 服务器实现
    class Impl;
    std::unique_ptr<Impl> impl_;
  };

  // 为了向后兼容，保留原来的StatsApiServer类名
  using StatsApiServer = ApiServer;

} // namespace scheduler