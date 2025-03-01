#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include "stats_manager.h"

namespace scheduler
{
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
                                     const std::map<std::string, std::string> &query_params);

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
   * @brief 简单的HTTP服务器，用于提供统计信息API
   */
  class StatsApiServer
  {
  public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     */
    explicit StatsApiServer(int port);

    /**
     * @brief 析构函数
     */
    ~StatsApiServer();

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

} // namespace scheduler