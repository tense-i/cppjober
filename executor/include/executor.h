#pragma once

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include "job.h"
#include "kafka_message_queue.h"

namespace scheduler
{

  class JobExecutor
  {
  public:
    JobExecutor(const std::string &executor_id);
    virtual ~JobExecutor();

    // 启动执行器
    void start();
    // 停止执行器
    void stop();

  protected:
    // 执行线程函数
    void execute_loop();
    // 执行具体任务
    JobResult execute_job(const JobInfo &job);
    // 向调度中心注册
    virtual void register_executor();
    // 向调度中心注销
    virtual void unregister_executor();
    // 心跳检测
    void heartbeat_loop();
    // 取消任务
    void cancel_job(const std::string &job_id);
    // 检查任务是否被取消
    bool is_job_cancelled(const std::string &job_id);

    std::string executor_id_;
    std::unique_ptr<KafkaMessageQueue> kafka_client_;

    std::atomic<bool> running_;
    std::thread execute_thread_;
    std::thread heartbeat_thread_;
    std::queue<JobInfo> job_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;

    // 取消任务列表
    std::unordered_set<std::string> cancelled_jobs_;
    std::mutex cancel_mutex_;
  };

} // namespace scheduler