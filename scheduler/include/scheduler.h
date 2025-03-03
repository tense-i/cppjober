#pragma once

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "job.h"
#include "job_dao.h"
#include "kafka_message_queue.h"
#include "zk_registry.h"

namespace scheduler
{
  // 前向声明
  class JobQueue;
  class ExecutorRegistry;

  // 执行器选择策略
  enum class ExecutorSelectionStrategy
  {
    RANDOM,      // 随机选择
    ROUND_ROBIN, // 轮询
    LEAST_LOAD   // 最少负载
  };

  class JobScheduler
  {
  public:
    // 构造函数
    JobScheduler(const std::string &node_id, const std::string &zk_hosts);
    ~JobScheduler();

    // 启动调度器
    void start();
    // 停止调度器
    void stop();

    // 提交任务
    std::string submit_job(const JobInfo &job);
    // 取消任务
    bool cancel_job(const std::string &job_id);
    // 获取任务状态
    JobStatus get_job_status(const std::string &job_id);
    // 获取任务结果
    JobResult get_job_result(const std::string &job_id);

    // 设置执行器选择策略
    void set_executor_selection_strategy(ExecutorSelectionStrategy strategy);
    // 获取当前执行器选择策略
    ExecutorSelectionStrategy get_executor_selection_strategy() const;

    // 获取节点状态
    bool is_leader() const;
    std::string get_node_id() const;

  private:
    // 调度线程函数
    void schedule_loop();
    // 检查任务是否需要执行
    bool should_execute(const JobInfo &job);
    // 分发任务到执行器
    void dispatch_job(const JobInfo &job);
    // 处理执行结果
    void handle_result(const JobResult &result);

    // 主备切换相关
    void leader_election_loop();
    bool try_become_leader();
    void handle_leader_change(const std::string &new_leader);
    void on_become_leader();
    void on_become_follower();

    std::unique_ptr<JobQueue> job_queue_;
    std::unique_ptr<ExecutorRegistry> executor_registry_;
    std::unique_ptr<JobDAO> job_storage_;
    std::unique_ptr<KafkaMessageQueue> kafka_client_;
    std::shared_ptr<ZkRegistry> zk_registry_;

    bool running_;
    std::thread schedule_thread_;
    std::thread election_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    // 执行器选择策略
    ExecutorSelectionStrategy executor_selection_strategy_;

    // 节点标识
    std::string node_id_;
    bool is_leader_;
  };

} // namespace scheduler