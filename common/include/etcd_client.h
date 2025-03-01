#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <etcd/Client.hpp>
#include "job.h"

namespace scheduler
{

  class EtcdClient
  {
  public:
    using WatchCallback = std::function<void(const std::string &, const std::string &)>;

    EtcdClient(const std::string &endpoints);
    ~EtcdClient();

    // 注册执行器
    bool register_executor(const std::string &executor_id, const std::string &address);
    // 注销执行器
    bool unregister_executor(const std::string &executor_id);
    // 获取所有执行器
    std::vector<std::string> get_executors();
    // 更新执行器心跳
    bool update_heartbeat(const std::string &executor_id);
    // 监听执行器变化
    void watch_executors(WatchCallback callback);
    // 停止监听
    void stop_watch();

    // 分布式锁
    bool try_lock(const std::string &lock_key, int ttl_seconds);
    void unlock(const std::string &lock_key);

  private:
    std::unique_ptr<etcd::Client> client_;
    std::string endpoints_;
    bool running_;
  };

} // namespace scheduler