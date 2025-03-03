#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <zookeeper/zookeeper.h>

namespace scheduler
{

  // ZooKeeper事件回调函数类型
  using ZkWatchCallback = std::function<void(const std::string &path, const std::string &data)>;

  class ZkClient
  {
  public:
    // 构造函数
    explicit ZkClient(const std::string &hosts, int timeout_ms = 30000);
    ~ZkClient();

    // 连接管理
    bool connect();
    void disconnect();
    bool is_connected() const;

    // 节点操作
    bool create_node(const std::string &path, const std::string &data,
                     int flags = 0, bool recursive = false);
    bool delete_node(const std::string &path, bool recursive = false);
    bool exists(const std::string &path, bool watch = false);
    std::string get_data(const std::string &path, bool watch = false);
    bool set_data(const std::string &path, const std::string &data);
    std::vector<std::string> get_children(const std::string &path, bool watch = false);

    // 监听器
    void add_watch(const std::string &path, ZkWatchCallback callback);
    void remove_watch(const std::string &path);

    // 分布式锁
    bool try_lock(const std::string &path, int timeout_ms = 5000);
    void unlock(const std::string &path);

  private:
    // ZooKeeper会话句柄
    zhandle_t *zk_handle_;

    // 连接参数
    std::string hosts_;
    int timeout_ms_;

    // 监听器回调映射
    std::map<std::string, ZkWatchCallback> watch_callbacks_;

    // 全局监听器
    static void global_watcher(zhandle_t *zh, int type, int state,
                               const char *path, void *watcherCtx);

    // 辅助函数
    bool ensure_path(const std::string &path);
    void handle_watch_event(int type, int state, const char *path);

    // 禁止拷贝和赋值
    ZkClient(const ZkClient &) = delete;
    ZkClient &operator=(const ZkClient &) = delete;
  };

} // namespace scheduler