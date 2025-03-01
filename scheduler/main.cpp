#include "scheduler.h"
#include "config_manager.h"
#include "stats_api.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>
#include <filesystem>

using namespace scheduler;

// 全局调度器实例
std::unique_ptr<JobScheduler> g_scheduler;
// 全局统计信息API服务器实例
std::unique_ptr<StatsApiServer> g_stats_server;

// 信号处理函数
void signalHandler(int signal)
{
  spdlog::info("接收到信号: {}, 正在停止调度器...", signal);
  if (g_stats_server)
  {
    g_stats_server->stop();
  }
  if (g_scheduler)
  {
    g_scheduler->stop();
  }
  exit(0);
}

int main(int argc, char *argv[])
{
  try
  {
    // 设置日志级别
    spdlog::set_level(spdlog::level::info);
    spdlog::info("调度器启动中...");

    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 加载配置文件
    std::string configFile = "config/scheduler.conf";

    // 检查命令行参数中是否指定了配置文件
    if (argc > 1)
    {
      configFile = argv[1];
    }

    // 检查配置文件是否存在
    if (!std::filesystem::exists(configFile))
    {
      spdlog::warn("配置文件 {} 不存在，将使用默认配置", configFile);
    }
    else
    {
      // 加载配置文件
      if (ConfigManager::getInstance().loadFromFile(configFile))
      {
        spdlog::info("成功加载配置文件: {}", configFile);
      }
      else
      {
        spdlog::warn("加载配置文件失败: {}, 将使用默认配置", configFile);
      }
    }

    // 创建并启动调度器
    g_scheduler = std::make_unique<JobScheduler>();
    g_scheduler->start();

    // 输出当前使用的执行器选择策略
    auto strategy = g_scheduler->get_executor_selection_strategy();
    std::string strategyName;
    switch (strategy)
    {
    case ExecutorSelectionStrategy::RANDOM:
      strategyName = "随机选择";
      break;
    case ExecutorSelectionStrategy::ROUND_ROBIN:
      strategyName = "轮询";
      break;
    case ExecutorSelectionStrategy::LEAST_LOAD:
      strategyName = "最少负载";
      break;
    }
    spdlog::info("当前执行器选择策略: {}", strategyName);

    // 创建并启动统计信息API服务器
    int statsPort = ConfigManager::getInstance().getInt("stats.api.port", 8080);
    g_stats_server = std::make_unique<StatsApiServer>(statsPort);
    g_stats_server->start();
    spdlog::info("统计信息API服务器已启动，访问 http://localhost:{}/api/stats 查看统计信息", statsPort);

    // 主线程等待
    spdlog::info("调度器已启动，按Ctrl+C停止");
    while (true)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  catch (const std::exception &e)
  {
    spdlog::error("调度器异常: {}", e.what());
    return 1;
  }

  return 0;
}