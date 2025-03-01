#include "executor.h"
#include "config_manager.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>
#include <filesystem>
#include <uuid/uuid.h>

using namespace scheduler;

// 全局执行器实例
std::unique_ptr<JobExecutor> g_executor;

// 生成UUID
std::string generate_uuid()
{
  uuid_t uuid;
  char uuid_str[37];

  uuid_generate(uuid);
  uuid_unparse_lower(uuid, uuid_str);

  return std::string(uuid_str);
}

// 信号处理函数
void signalHandler(int signal)
{
  spdlog::info("接收到信号: {}, 正在停止执行器...", signal);
  if (g_executor)
  {
    g_executor->stop();
  }
  exit(0);
}

int main(int argc, char *argv[])
{
  try
  {
    // 设置日志级别
    spdlog::set_level(spdlog::level::info);
    spdlog::info("执行器启动中...");

    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 加载配置文件
    std::string configFile = "config/executor.conf";

    // 检查命令行参数中是否指定了配置文件
    if (argc > 1)
    {
      configFile = argv[1];
    }

    // 检查配置文件是否存在
    if (!std::filesystem::exists(configFile))
    {
      // 尝试使用调度器的配置文件
      configFile = "config/scheduler.conf";
      if (!std::filesystem::exists(configFile))
      {
        spdlog::warn("配置文件不存在，将使用默认配置");
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

    // 生成执行器ID
    std::string executor_id = generate_uuid();
    spdlog::info("执行器ID: {}", executor_id);

    // 创建并启动执行器
    g_executor = std::make_unique<JobExecutor>(executor_id);
    g_executor->start();

    // 主线程等待
    spdlog::info("执行器已启动，按Ctrl+C停止");
    while (true)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  catch (const std::exception &e)
  {
    spdlog::error("执行器异常: {}", e.what());
    return 1;
  }

  return 0;
}