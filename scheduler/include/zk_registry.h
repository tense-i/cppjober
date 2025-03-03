#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "zk_client.h"
#include "job.h"

namespace scheduler
{

  class ZkRegistry
  {
  public:
    // 构造函数
    explicit ZkRegistry(std::shared_ptr<ZkClient> zk_client);
    ~ZkRegistry() = default;

    // 初始化ZooKeeper节点结构
    bool init();

    // 执行器注册
    bool register_executor(const ExecutorInfo &executor);
    bool unregister_executor(const std::string &executor_id);

    // 获取执行器信息
    std::vector<ExecutorInfo> get_executors();
    std::optional<ExecutorInfo> get_executor(const std::string &executor_id);

    // 更新执行器状态
    bool update_executor_status(const std::string &executor_id, bool online);
    bool update_executor_load(const std::string &executor_id, int current_load);

    // 监听执行器变化
    void watch_executors(std::function<void(const std::vector<ExecutorInfo> &)> callback);

    // 选举主节点
    bool elect_leader(const std::string &node_id);
    bool is_leader(const std::string &node_id) const;
    std::string get_current_leader() const;

    // 分布式锁
    bool acquire_lock(const std::string &lock_name, int timeout_ms = 5000);
    void release_lock(const std::string &lock_name);

  private:
    // ZooKeeper路径常量
    static constexpr const char *ROOT_PATH = "/scheduler";
    static constexpr const char *EXECUTORS_PATH = "/scheduler/executors";
    static constexpr const char *LEADER_PATH = "/scheduler/leader";
    static constexpr const char *LOCKS_PATH = "/scheduler/locks";

    // ZooKeeper客户端
    std::shared_ptr<ZkClient> zk_client_;

    // 辅助函数
    std::string get_executor_path(const std::string &executor_id) const;
    std::string get_lock_path(const std::string &lock_name) const;
    ExecutorInfo parse_executor_data(const std::string &data) const;
    std::string serialize_executor_data(const ExecutorInfo &executor) const;

    // 禁止拷贝和赋值
    ZkRegistry(const ZkRegistry &) = delete;
    ZkRegistry &operator=(const ZkRegistry &) = delete;
  };

} // namespace scheduler