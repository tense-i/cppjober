#include "executor_registry.h"
#include <spdlog/spdlog.h>
#include <random>
#include <algorithm>

namespace scheduler
{

  ExecutorRegistry::ExecutorRegistry(JobDAO &dao, std::shared_ptr<ZkRegistry> zk_registry)
      : dao_(dao), zk_registry_(std::move(zk_registry)), current_index_(0)
  {
    // 初始化时同步执行器信息
    syncExecutorsFromZk();

    // 监听执行器变化
    watchExecutorChanges();
  }

  std::optional<std::pair<std::string, std::string>> ExecutorRegistry::getRandomExecutor()
  {
    auto executors = dao_.getOnlineExecutors();
    if (executors.empty())
    {
      return std::nullopt;
    }

    // 随机选择一个执行器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, executors.size() - 1);
    int index = dis(gen);

    return executors[index];
  }

  std::optional<std::pair<std::string, std::string>> ExecutorRegistry::getRoundRobinExecutor()
  {
    auto executors = dao_.getOnlineExecutors();
    if (executors.empty())
    {
      return std::nullopt;
    }

    // 使用轮询策略选择执行器
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_index_ >= executors.size())
    {
      current_index_ = 0;
    }
    auto result = executors[current_index_];
    current_index_ = (current_index_ + 1) % executors.size();

    return result;
  }

  std::optional<std::pair<std::string, std::string>> ExecutorRegistry::getLeastLoadExecutor()
  {
    auto executors = dao_.getOnlineExecutorsWithLoad();
    if (executors.empty())
    {
      return std::nullopt;
    }

    // 找到负载最小的执行器
    auto min_load_executor = std::min_element(executors.begin(), executors.end(),
                                              [](const ExecutorInfo &a, const ExecutorInfo &b)
                                              {
                                                // 计算负载比例
                                                float load_ratio_a = a.max_load > 0 ? static_cast<float>(a.current_load) / a.max_load : 1.0f;
                                                float load_ratio_b = b.max_load > 0 ? static_cast<float>(b.current_load) / b.max_load : 1.0f;
                                                return load_ratio_a < load_ratio_b;
                                              });

    // 检查是否超过最大负载
    if (min_load_executor->current_load >= min_load_executor->max_load)
    {
      spdlog::warn("All executors are at maximum load capacity");
      return std::nullopt;
    }

    return std::make_pair(min_load_executor->executor_id, min_load_executor->address);
  }

  std::optional<std::pair<std::string, std::string>> ExecutorRegistry::getAvailableExecutor(
      ExecutorSelectionStrategy strategy)
  {
    switch (strategy)
    {
    case ExecutorSelectionStrategy::ROUND_ROBIN:
      return getRoundRobinExecutor();
    case ExecutorSelectionStrategy::LEAST_LOAD:
      return getLeastLoadExecutor();
    case ExecutorSelectionStrategy::RANDOM:
    default:
      return getRandomExecutor();
    }
  }

  std::optional<std::pair<std::string, std::string>> ExecutorRegistry::getAvailableExecutor()
  {
    return getRandomExecutor();
  }

  bool ExecutorRegistry::updateExecutorLoad(const std::string &executorId, bool increment)
  {
    // 更新数据库中的负载信息
    bool success = increment ? dao_.incrementExecutorLoad(executorId) : dao_.decrementExecutorLoad(executorId);

    if (success)
    {
      // 获取更新后的执行器信息
      auto executor = dao_.getExecutorInfo(executorId);
      if (executor)
      {
        // 同步到ZooKeeper
        zk_registry_->update_executor_load(executorId, executor->current_load);
      }
    }

    return success;
  }

  bool ExecutorRegistry::incrementExecutorTaskCount(const std::string &executorId)
  {
    return dao_.incrementExecutorTaskCount(executorId);
  }

  void ExecutorRegistry::syncExecutorsFromZk()
  {
    try
    {
      // 从ZooKeeper获取所有执行器信息
      auto zk_executors = zk_registry_->get_executors();

      // 更新到数据库
      for (const auto &executor : zk_executors)
      {
        dao_.updateExecutor(executor);
      }

      spdlog::info("Synchronized {} executors from ZooKeeper", zk_executors.size());
    }
    catch (const std::exception &e)
    {
      spdlog::error("Failed to sync executors from ZooKeeper: {}", e.what());
    }
  }

  void ExecutorRegistry::watchExecutorChanges()
  {
    // 监听执行器变化
    zk_registry_->watch_executors([this](const std::vector<ExecutorInfo> &executors)
                                  {
        try {
            // 更新数据库中的执行器信息
            for (const auto& executor : executors) {
                dao_.updateExecutor(executor);
            }
            
            spdlog::info("Updated {} executors from ZooKeeper notification", executors.size());
        } catch (const std::exception& e) {
            spdlog::error("Failed to handle executor changes: {}", e.what());
        } });
  }

} // namespace scheduler