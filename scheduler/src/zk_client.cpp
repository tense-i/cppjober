#include "zk_client.h"
#include <spdlog/spdlog.h>
#include <string.h>
#include <sys/stat.h>

namespace scheduler
{

  ZkClient::ZkClient(const std::string &hosts, int timeout_ms)
      : zk_handle_(nullptr), hosts_(hosts), timeout_ms_(timeout_ms)
  {
  }

  ZkClient::~ZkClient()
  {
    disconnect();
  }

  bool ZkClient::connect()
  {
    if (zk_handle_)
    {
      return true;
    }

    zk_handle_ = zookeeper_init(hosts_.c_str(), global_watcher,
                                timeout_ms_, nullptr, this, 0);
    if (!zk_handle_)
    {
      spdlog::error("Failed to initialize ZooKeeper client: {}", strerror(errno));
      return false;
    }

    // 等待连接建立
    int retry = 10;
    while (retry-- > 0)
    {
      if (is_connected())
      {
        spdlog::info("Connected to ZooKeeper server: {}", hosts_);
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::error("Failed to connect to ZooKeeper server: {}", hosts_);
    return false;
  }

  void ZkClient::disconnect()
  {
    if (zk_handle_)
    {
      zookeeper_close(zk_handle_);
      zk_handle_ = nullptr;
    }
  }

  bool ZkClient::is_connected() const
  {
    if (!zk_handle_)
    {
      return false;
    }
    int state = zoo_state(zk_handle_);
    return state == ZOO_CONNECTED_STATE;
  }

  bool ZkClient::create_node(const std::string &path, const std::string &data,
                             int flags, bool recursive)
  {
    if (!is_connected())
    {
      return false;
    }

    if (recursive && !ensure_path(path))
    {
      return false;
    }

    int ret = zoo_create(zk_handle_, path.c_str(), data.c_str(), data.length(),
                         &ZOO_OPEN_ACL_UNSAFE, flags, nullptr, 0);
    if (ret != ZOK)
    {
      if (ret != ZNODEEXISTS)
      {
        spdlog::error("Failed to create node {}: {}", path, zerror(ret));
        return false;
      }
      // 节点已存在时,如果不是临时节点则更新数据
      if (!(flags & ZOO_EPHEMERAL))
      {
        return set_data(path, data);
      }
    }
    return true;
  }

  bool ZkClient::delete_node(const std::string &path, bool recursive)
  {
    if (!is_connected())
    {
      return false;
    }

    if (recursive)
    {
      std::vector<std::string> children = get_children(path, false);
      for (const auto &child : children)
      {
        if (!delete_node(path + "/" + child, true))
        {
          return false;
        }
      }
    }

    int ret = zoo_delete(zk_handle_, path.c_str(), -1);
    if (ret != ZOK && ret != ZNONODE)
    {
      spdlog::error("Failed to delete node {}: {}", path, zerror(ret));
      return false;
    }
    return true;
  }

  bool ZkClient::exists(const std::string &path, bool watch)
  {
    if (!is_connected())
    {
      return false;
    }

    Stat stat;
    int ret = zoo_exists(zk_handle_, path.c_str(), watch, &stat);
    return ret == ZOK;
  }

  std::string ZkClient::get_data(const std::string &path, bool watch)
  {
    if (!is_connected())
    {
      return "";
    }

    char buffer[4096];
    int buffer_len = sizeof(buffer);
    Stat stat;
    int ret = zoo_get(zk_handle_, path.c_str(), watch, buffer, &buffer_len, &stat);
    if (ret != ZOK)
    {
      spdlog::error("Failed to get data from {}: {}", path, zerror(ret));
      return "";
    }
    return std::string(buffer, buffer_len);
  }

  bool ZkClient::set_data(const std::string &path, const std::string &data)
  {
    if (!is_connected())
    {
      return false;
    }

    int ret = zoo_set(zk_handle_, path.c_str(), data.c_str(),
                      data.length(), -1);
    if (ret != ZOK)
    {
      spdlog::error("Failed to set data for {}: {}", path, zerror(ret));
      return false;
    }
    return true;
  }

  std::vector<std::string> ZkClient::get_children(const std::string &path, bool watch)
  {
    if (!is_connected())
    {
      return {};
    }

    String_vector children;
    int ret = zoo_get_children(zk_handle_, path.c_str(), watch, &children);
    if (ret != ZOK)
    {
      spdlog::error("Failed to get children of {}: {}", path, zerror(ret));
      return {};
    }

    std::vector<std::string> result;
    for (int i = 0; i < children.count; ++i)
    {
      result.push_back(children.data[i]);
    }
    deallocate_String_vector(&children);
    return result;
  }

  void ZkClient::add_watch(const std::string &path, ZkWatchCallback callback)
  {
    watch_callbacks_[path] = std::move(callback);
    exists(path, true);
  }

  void ZkClient::remove_watch(const std::string &path)
  {
    watch_callbacks_.erase(path);
  }

  bool ZkClient::try_lock(const std::string &path, int timeout_ms)
  {
    if (!is_connected())
    {
      return false;
    }

    std::string lock_path = path + "/_lock_";
    auto start_time = std::chrono::steady_clock::now();

    while (true)
    {
      // 尝试创建临时节点
      if (create_node(lock_path, "", ZOO_EPHEMERAL))
      {
        return true;
      }

      // 检查是否超时
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - start_time)
                         .count();
      if (elapsed >= timeout_ms)
      {
        return false;
      }

      // 等待一段时间后重试
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void ZkClient::unlock(const std::string &path)
  {
    if (!is_connected())
    {
      return;
    }

    std::string lock_path = path + "/_lock_";
    delete_node(lock_path, false);
  }

  void ZkClient::global_watcher(zhandle_t *zh, int type, int state,
                                const char *path, void *watcherCtx)
  {
    if (watcherCtx)
    {
      ZkClient *client = static_cast<ZkClient *>(watcherCtx);
      client->handle_watch_event(type, state, path);
    }
  }

  bool ZkClient::ensure_path(const std::string &path)
  {
    if (path.empty() || path[0] != '/')
    {
      return false;
    }

    size_t pos = 1;
    while (pos < path.length())
    {
      pos = path.find('/', pos);
      if (pos == std::string::npos)
      {
        pos = path.length();
      }
      std::string subpath = path.substr(0, pos);
      if (!exists(subpath, false))
      {
        if (!create_node(subpath, "", 0, false))
        {
          return false;
        }
      }
      ++pos;
    }
    return true;
  }

  void ZkClient::handle_watch_event(int type, int state, const char *path)
  {
    if (!path)
    {
      return;
    }

    // 处理会话状态变化
    if (type == ZOO_SESSION_EVENT)
    {
      if (state == ZOO_CONNECTED_STATE)
      {
        spdlog::info("Connected to ZooKeeper server");
      }
      else if (state == ZOO_EXPIRED_SESSION_STATE)
      {
        spdlog::warn("ZooKeeper session expired");
        disconnect();
        connect();
      }
      return;
    }

    // 处理节点事件
    auto it = watch_callbacks_.find(path);
    if (it != watch_callbacks_.end())
    {
      std::string data;
      if (exists(path, true))
      {
        data = get_data(path, true);
      }
      it->second(path, data);
    }
  }

} // namespace scheduler