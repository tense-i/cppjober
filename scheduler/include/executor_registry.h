#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "job_dao.h"
#include "zk_registry.h"

namespace scheduler
{

  class ExecutorRegistry
  {
  public:
    // 构造函数
    ExecutorRegistry(JobDAO &dao, std::shared_ptr<ZkRegistry> zk_registry);
    ~ExecutorRegistry() = default;

    // 获取可用执行器 - 随机策略
    std::optional<std::pair<std::string, std::string>> getRandomExecutor();

    // 获取可用执行器 - 轮询策略
    std::optional<std::pair<std::string, std::string>> getRoundRobinExecutor();

    // 获取可用执行器 - 最少负载策略
    std::optional<std::pair<std::string, std::string>> getLeastLoadExecutor();

    // 获取可用执行器 - 根据策略选择
    std::optional<std::pair<std::string, std::string>> getAvailableExecutor(ExecutorSelectionStrategy strategy);

    // 兼容旧接口
    std::optional<std::pair<std::string, std::string>> getAvailableExecutor();

    // 更新执行器负载
    bool updateExecutorLoad(const std::string &executorId, bool increment);

    // 增加执行器任务计数
    bool incrementExecutorTaskCount(const std::string &executorId);

  private:
    JobDAO &dao_;
    std::shared_ptr<ZkRegistry> zk_registry_;
    size_t current_index_; // 用于轮询策略
    std::mutex mutex_;     // 保护current_index_

    // 从ZooKeeper同步执行器信息到数据库
    void syncExecutorsFromZk();

    // 监听执行器变化
    void watchExecutorChanges();
  };

} // namespace scheduler