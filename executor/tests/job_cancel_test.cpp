#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include "executor.h"
#include "job.h"
#include "kafka_message_queue.h"
#include "mock_executor.h"

using namespace scheduler;
using namespace testing;
using namespace std::chrono_literals;

// 测试夹具
class JobCancelTest : public Test
{
protected:
  void SetUp() override
  {
    executor = std::make_unique<MockExecutor>("test-executor-001");
    EXPECT_CALL(*executor, register_executor()).Times(0);
    EXPECT_CALL(*executor, unregister_executor()).Times(0);
  }

  void TearDown() override
  {
    executor.reset();
  }

  // 创建测试任务
  JobInfo create_test_job(const std::string &job_id)
  {
    JobInfo job;
    job.job_id = job_id;
    job.name = "Test Job " + job_id;
    job.command = "echo 'Test job " + job_id + "'";
    job.type = JobType::ONCE;
    job.priority = 0;
    job.timeout = 10;
    return job;
  }

  std::unique_ptr<MockExecutor> executor;
};

// 测试取消不在队列中的任务
TEST_F(JobCancelTest, CancelNonExistingJob)
{
  // 取消一个不存在的任务
  std::string job_id = "non-existing-job-001";
  executor->cancel_job(job_id);

  // 验证任务被标记为已取消
  EXPECT_TRUE(executor->is_job_cancelled(job_id));
}

// 测试取消队列中的任务
TEST_F(JobCancelTest, CancelQueuedJob)
{
  // 创建测试任务
  std::string job_id = "queued-job-001";
  JobInfo job = create_test_job(job_id);

  // 添加任务到队列
  executor->add_job_to_queue(job);
  EXPECT_EQ(executor->get_queue_size(), 1);
  EXPECT_TRUE(executor->queue_contains_job(job_id));

  // 取消任务
  executor->cancel_job(job_id);

  // 验证任务被标记为已取消并从队列中移除
  EXPECT_TRUE(executor->is_job_cancelled(job_id));
  EXPECT_EQ(executor->get_queue_size(), 0);
  EXPECT_FALSE(executor->queue_contains_job(job_id));
}

// 测试取消多个任务
TEST_F(JobCancelTest, CancelMultipleJobs)
{
  // 创建多个测试任务
  std::vector<std::string> job_ids = {
      "multi-job-001", "multi-job-002", "multi-job-003"};

  // 添加任务到队列
  for (const auto &job_id : job_ids)
  {
    executor->add_job_to_queue(create_test_job(job_id));
  }
  EXPECT_EQ(executor->get_queue_size(), 3);

  // 取消第一个和最后一个任务
  executor->cancel_job(job_ids[0]);
  executor->cancel_job(job_ids[2]);

  // 验证任务状态
  EXPECT_TRUE(executor->is_job_cancelled(job_ids[0]));
  EXPECT_FALSE(executor->is_job_cancelled(job_ids[1]));
  EXPECT_TRUE(executor->is_job_cancelled(job_ids[2]));

  // 验证队列状态
  EXPECT_EQ(executor->get_queue_size(), 1);
  EXPECT_FALSE(executor->queue_contains_job(job_ids[0]));
  EXPECT_TRUE(executor->queue_contains_job(job_ids[1]));
  EXPECT_FALSE(executor->queue_contains_job(job_ids[2]));
}

// 测试取消已经取消的任务
TEST_F(JobCancelTest, CancelAlreadyCancelledJob)
{
  std::string job_id = "already-cancelled-job-001";

  // 第一次取消
  executor->cancel_job(job_id);
  EXPECT_TRUE(executor->is_job_cancelled(job_id));

  // 第二次取消
  executor->cancel_job(job_id);
  EXPECT_TRUE(executor->is_job_cancelled(job_id));
}

// 主函数
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}