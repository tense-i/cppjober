#include "zk_registry.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace scheduler
{

  ZkRegistry::ZkRegistry(std::shared_ptr<ZkClient> zk_client)
      : zk_client_(std::move(zk_client))
  {
  }

  bool ZkRegistry::init()
  {
    // 创建根节点
    if (!zk_client_->create_node(ROOT_PATH, "", 0, true))
    {
      return false;
    }

    // 创建执行器节点目录
    if (!zk_client_->create_node(EXECUTORS_PATH, "", 0, true))
    {
      return false;
    }

    // 创建Leader节点目录
    if (!zk_client_->create_node(LEADER_PATH, "", 0, true))
    {
      return false;
    }

    // 创建锁节点目录
    if (!zk_client_->create_node(LOCKS_PATH, "", 0, true))
    {
      return false;
    }

    return true;
  }

  bool ZkRegistry::register_executor(const ExecutorInfo &executor)
  {
    std::string path = get_executor_path(executor.executor_id);
    std::string data = serialize_executor_data(executor);

    // 创建临时节点
    return zk_client_->create_node(path, data, ZOO_EPHEMERAL);
  }

  bool ZkRegistry::unregister_executor(const std::string &executor_id)
  {
    std::string path = get_executor_path(executor_id);
    return zk_client_->delete_node(path);
  }

  std::vector<ExecutorInfo> ZkRegistry::get_executors()
  {
    std::vector<ExecutorInfo> executors;
    auto children = zk_client_->get_children(EXECUTORS_PATH);

    for (const auto &child : children)
    {
      std::string path = get_executor_path(child);
      std::string data = zk_client_->get_data(path);
      if (!data.empty())
      {
        try
        {
          executors.push_back(parse_executor_data(data));
        }
        catch (const std::exception &e)
        {
          spdlog::error("Failed to parse executor data: {}", e.what());
        }
      }
    }

    return executors;
  }

  std::optional<ExecutorInfo> ZkRegistry::get_executor(const std::string &executor_id)
  {
    std::string path = get_executor_path(executor_id);
    std::string data = zk_client_->get_data(path);

    if (!data.empty())
    {
      try
      {
        return parse_executor_data(data);
      }
      catch (const std::exception &e)
      {
        spdlog::error("Failed to parse executor data: {}", e.what());
      }
    }

    return std::nullopt;
  }

  bool ZkRegistry::update_executor_status(const std::string &executor_id, bool online)
  {
    auto executor = get_executor(executor_id);
    if (!executor)
    {
      return false;
    }

    executor->online = online;
    std::string path = get_executor_path(executor_id);
    return zk_client_->set_data(path, serialize_executor_data(*executor));
  }

  bool ZkRegistry::update_executor_load(const std::string &executor_id, int current_load)
  {
    auto executor = get_executor(executor_id);
    if (!executor)
    {
      return false;
    }

    executor->current_load = current_load;
    std::string path = get_executor_path(executor_id);
    return zk_client_->set_data(path, serialize_executor_data(*executor));
  }

  void ZkRegistry::watch_executors(std::function<void(const std::vector<ExecutorInfo> &)> callback)
  {
    zk_client_->add_watch(EXECUTORS_PATH, [this, callback](const std::string &path, const std::string &data)
                          { callback(get_executors()); });
  }

  bool ZkRegistry::elect_leader(const std::string &node_id)
  {
    // 尝试创建临时节点
    return zk_client_->create_node(LEADER_PATH, node_id, ZOO_EPHEMERAL);
  }

  bool ZkRegistry::is_leader(const std::string &node_id) const
  {
    std::string current_leader = get_current_leader();
    return !current_leader.empty() && current_leader == node_id;
  }

  std::string ZkRegistry::get_current_leader() const
  {
    return zk_client_->get_data(LEADER_PATH);
  }

  bool ZkRegistry::acquire_lock(const std::string &lock_name, int timeout_ms)
  {
    std::string path = get_lock_path(lock_name);
    return zk_client_->try_lock(path, timeout_ms);
  }

  void ZkRegistry::release_lock(const std::string &lock_name)
  {
    std::string path = get_lock_path(lock_name);
    zk_client_->unlock(path);
  }

  std::string ZkRegistry::get_executor_path(const std::string &executor_id) const
  {
    return EXECUTORS_PATH + "/" + executor_id;
  }

  std::string ZkRegistry::get_lock_path(const std::string &lock_name) const
  {
    return LOCKS_PATH + "/" + lock_name;
  }

  ExecutorInfo ZkRegistry::parse_executor_data(const std::string &data) const
  {
    nlohmann::json j = nlohmann::json::parse(data);
    ExecutorInfo executor;

    executor.executor_id = j["executor_id"].get<std::string>();
    executor.address = j["address"].get<std::string>();
    executor.current_load = j["current_load"].get<int>();
    executor.max_load = j["max_load"].get<int>();
    executor.total_tasks_executed = j["total_tasks_executed"].get<int>();
    executor.online = j["online"].get<bool>();

    // 解析时间戳
    int64_t timestamp = j["last_heartbeat"].get<int64_t>();
    executor.last_heartbeat = std::chrono::system_clock::time_point(
        std::chrono::seconds(timestamp));

    return executor;
  }

  std::string ZkRegistry::serialize_executor_data(const ExecutorInfo &executor) const
  {
    nlohmann::json j;

    j["executor_id"] = executor.executor_id;
    j["address"] = executor.address;
    j["current_load"] = executor.current_load;
    j["max_load"] = executor.max_load;
    j["total_tasks_executed"] = executor.total_tasks_executed;
    j["online"] = executor.online;

    // 序列化时间戳
    j["last_heartbeat"] = std::chrono::duration_cast<std::chrono::seconds>(
                              executor.last_heartbeat.time_since_epoch())
                              .count();

    return j.dump();
  }

} // namespace scheduler