#pragma once

#include "executor.h"
#include <gmock/gmock.h>

namespace scheduler
{

  class MockExecutor : public JobExecutor
  {
  public:
    MockExecutor(const std::string &executor_id) : JobExecutor(executor_id) {}

    // 重写protected方法，避免实际的数据库和Kafka操作
    MOCK_METHOD(void, register_executor, (), (override));
    MOCK_METHOD(void, unregister_executor, (), (override));

    // 暴露protected方法供测试使用
    using JobExecutor::cancel_job;
    using JobExecutor::is_job_cancelled;

    // 添加任务到队列
    void add_job_to_queue(const JobInfo &job)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      job_queue_.push(job);
      cv_.notify_one();
    }

    // 获取队列大小
    size_t get_queue_size()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return job_queue_.size();
    }

    // 检查队列中是否包含指定ID的任务
    bool queue_contains_job(const std::string &job_id)
    {
      std::lock_guard<std::mutex> lock(mutex_);

      std::queue<JobInfo> temp_queue = job_queue_;
      while (!temp_queue.empty())
      {
        if (temp_queue.front().job_id == job_id)
        {
          return true;
        }
        temp_queue.pop();
      }
      return false;
    }
  };

} // namespace scheduler