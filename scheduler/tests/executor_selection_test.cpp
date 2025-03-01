#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include "scheduler.h"
#include "job.h"
#include "job_dao.h"

using namespace scheduler;
using namespace testing;
using namespace std::chrono_literals;

// 模拟JobDAO类
class MockJobDAO : public JobDAO
{
public:
  MockJobDAO() : JobDAO() {}

  // 重写getOnlineExecutors方法
  std::vector<std::pair<std::string, std::string>> getOnlineExecutors() override
  {
    std::vector<std::pair<std::string, std::string>> executors;
    executors.emplace_back("executor-1", "localhost:8001");
    executors.emplace_back("executor-2", "localhost:8002");
    executors.emplace_back("executor-3", "localhost:8003");
    return executors;
  }

  // 重写getOnlineExecutorsWithLoad方法
  std::vector<ExecutorInfo> getOnlineExecutorsWithLoad() override
  {
    std::vector<ExecutorInfo> executors;

    ExecutorInfo info1;
    info1.executor_id = "executor-1";
    info1.address = "localhost:8001";
    info1.current_load = 5;
    info1.max_load = 10;
    info1.total_tasks_executed = 100;

    ExecutorInfo info2;
    info2.executor_id = "executor-2";
    info2.address = "localhost:8002";
    info2.current_load = 2;
    info2.max_load = 10;
    info2.total_tasks_executed = 80;

    ExecutorInfo info3;
    info3.executor_id = "executor-3";
    info3.address = "localhost:8003";
    info3.current_load = 8;
    info3.max_load = 10;
    info3.total_tasks_executed = 120;

    executors.push_back(info1);
    executors.push_back(info2);
    executors.push_back(info3);

    return executors;
  }

  // 重写其他需要的方法
  bool incrementExecutorLoad(const std::string &executorId) override { return true; }
  bool decrementExecutorLoad(const std::string &executorId) override { return true; }
  bool incrementExecutorTaskCount(const std::string &executorId) override { return true; }
};

// 测试夹具
class ExecutorSelectionTest : public Test
{
protected:
  void SetUp() override
  {
    dao = std::make_shared<MockJobDAO>();
    registry = std::make_unique<ExecutorRegistry>(*dao);
  }

  void TearDown() override
  {
    registry.reset();
    dao.reset();
  }

  std::shared_ptr<MockJobDAO> dao;
  std::unique_ptr<ExecutorRegistry> registry;
};

// 测试随机选择策略
TEST_F(ExecutorSelectionTest, RandomSelection)
{
  // 使用随机策略多次选择执行器
  std::map<std::string, int> selection_count;
  const int iterations = 1000;

  for (int i = 0; i < iterations; ++i)
  {
    auto executor = registry->getRandomExecutor();
    ASSERT_TRUE(executor.has_value());
    selection_count[executor->first]++;
  }

  // 验证所有执行器都被选择过
  EXPECT_GT(selection_count["executor-1"], 0);
  EXPECT_GT(selection_count["executor-2"], 0);
  EXPECT_GT(selection_count["executor-3"], 0);

  // 验证选择分布相对均匀（允许一定的随机偏差）
  const double expected_ratio = 1.0 / 3.0;
  const double tolerance = 0.1;

  for (const auto &[executor_id, count] : selection_count)
  {
    double ratio = static_cast<double>(count) / iterations;
    EXPECT_NEAR(ratio, expected_ratio, tolerance)
        << "Executor " << executor_id << " selection ratio: " << ratio;
  }
}

// 测试轮询选择策略
TEST_F(ExecutorSelectionTest, RoundRobinSelection)
{
  // 使用轮询策略多次选择执行器
  std::vector<std::string> selected_executors;
  const int iterations = 10;

  for (int i = 0; i < iterations; ++i)
  {
    auto executor = registry->getRoundRobinExecutor();
    ASSERT_TRUE(executor.has_value());
    selected_executors.push_back(executor->first);
  }

  // 验证轮询模式
  EXPECT_EQ(selected_executors[0], "executor-1");
  EXPECT_EQ(selected_executors[1], "executor-2");
  EXPECT_EQ(selected_executors[2], "executor-3");
  EXPECT_EQ(selected_executors[3], "executor-1");
  EXPECT_EQ(selected_executors[4], "executor-2");
  EXPECT_EQ(selected_executors[5], "executor-3");
  // 继续轮询
  EXPECT_EQ(selected_executors[6], "executor-1");
  EXPECT_EQ(selected_executors[7], "executor-2");
  EXPECT_EQ(selected_executors[8], "executor-3");
  EXPECT_EQ(selected_executors[9], "executor-1");
}

// 测试最少负载选择策略
TEST_F(ExecutorSelectionTest, LeastLoadSelection)
{
  // 使用最少负载策略选择执行器
  auto executor = registry->getLeastLoadExecutor();
  ASSERT_TRUE(executor.has_value());

  // 验证选择了负载最小的执行器（executor-2，负载为2）
  EXPECT_EQ(executor->first, "executor-2");
}

// 测试策略选择函数
TEST_F(ExecutorSelectionTest, StrategySelection)
{
  // 测试随机策略
  auto random_executor = registry->getAvailableExecutor(ExecutorSelectionStrategy::RANDOM);
  ASSERT_TRUE(random_executor.has_value());

  // 测试轮询策略
  auto round_robin_executor = registry->getAvailableExecutor(ExecutorSelectionStrategy::ROUND_ROBIN);
  ASSERT_TRUE(round_robin_executor.has_value());

  // 测试最少负载策略
  auto least_load_executor = registry->getAvailableExecutor(ExecutorSelectionStrategy::LEAST_LOAD);
  ASSERT_TRUE(least_load_executor.has_value());
  EXPECT_EQ(least_load_executor->first, "executor-2");
}

// 主函数
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}