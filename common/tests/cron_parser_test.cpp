#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include "cron_parser.h"

using namespace scheduler;
using namespace testing;
using namespace std::chrono_literals;

// 测试夹具
class CronParserTest : public Test
{
protected:
  void SetUp() override
  {
  }

  void TearDown() override
  {
  }

  // 创建指定时间的时间点
  std::chrono::system_clock::time_point makeTimePoint(int year, int month, int day, int hour, int minute, int second)
  {
    std::tm tm = {};
    tm.tm_year = year - 1900; // 年份从1900年开始
    tm.tm_mon = month - 1;    // 月份从0开始
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
  }
};

// 测试Cron表达式的有效性验证
TEST_F(CronParserTest, ValidateExpression)
{
  // 有效的表达式
  EXPECT_TRUE(CronParser::isValid("* * * * *"));
  EXPECT_TRUE(CronParser::isValid("0 0 * * *"));
  EXPECT_TRUE(CronParser::isValid("*/15 * * * *"));
  EXPECT_TRUE(CronParser::isValid("0 9-17 * * 1-5"));

  // 无效的表达式
  EXPECT_THROW(CronParser(""), std::invalid_argument);
  EXPECT_THROW(CronParser("* * *"), std::invalid_argument);
  EXPECT_THROW(CronParser("* * * * * *"), std::invalid_argument);
}

// 测试通配符匹配
TEST_F(CronParserTest, WildcardMatching)
{
  // "* * * * *" 表示每分钟执行
  CronParser parser("* * * * *");

  // 任何时间点都应该匹配
  auto time1 = makeTimePoint(2023, 1, 1, 0, 0, 0);
  auto time2 = makeTimePoint(2023, 6, 15, 12, 30, 0);
  auto time3 = makeTimePoint(2023, 12, 31, 23, 59, 0);

  EXPECT_TRUE(parser.matches(time1));
  EXPECT_TRUE(parser.matches(time2));
  EXPECT_TRUE(parser.matches(time3));
}

// 测试特定时间匹配
TEST_F(CronParserTest, SpecificTimeMatching)
{
  // "0 12 * * *" 表示每天中午12点执行
  CronParser parser("0 12 * * *");

  // 中午12点应该匹配
  auto time1 = makeTimePoint(2023, 1, 1, 12, 0, 0);
  EXPECT_TRUE(parser.matches(time1));

  // 其他时间不应该匹配
  auto time2 = makeTimePoint(2023, 1, 1, 12, 1, 0);
  auto time3 = makeTimePoint(2023, 1, 1, 11, 59, 0);
  EXPECT_FALSE(parser.matches(time2));
  EXPECT_FALSE(parser.matches(time3));
}

// 测试范围匹配
TEST_F(CronParserTest, RangeMatching)
{
  // "0 9-17 * * *" 表示每天9点到17点整点执行
  CronParser parser("0 9-17 * * *");

  // 9点到17点整点应该匹配
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 9, 0, 0)));
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 12, 0, 0)));
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 17, 0, 0)));

  // 其他时间不应该匹配
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 1, 8, 0, 0)));
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 1, 18, 0, 0)));
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 1, 9, 1, 0)));
}

// 测试步长匹配
TEST_F(CronParserTest, StepMatching)
{
  // "*/15 * * * *" 表示每15分钟执行一次
  CronParser parser("*/15 * * * *");

  // 0, 15, 30, 45分钟应该匹配
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 0, 0, 0)));
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 0, 15, 0)));
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 0, 30, 0)));
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 1, 0, 45, 0)));

  // 其他时间不应该匹配
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 1, 0, 5, 0)));
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 1, 0, 20, 0)));
}

// 测试星期匹配
TEST_F(CronParserTest, WeekdayMatching)
{
  // "0 12 * * 1-5" 表示工作日中午12点执行
  CronParser parser("0 12 * * 1-5");

  // 工作日中午12点应该匹配
  // 注意：2023年1月2日是星期一(1)，1月6日是星期五(5)
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 2, 12, 0, 0))); // 星期一
  EXPECT_TRUE(parser.matches(makeTimePoint(2023, 1, 6, 12, 0, 0))); // 星期五

  // 周末中午12点不应该匹配
  // 注意：2023年1月1日是星期日(0)，1月7日是星期六(6)
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 1, 12, 0, 0))); // 星期日
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 7, 12, 0, 0))); // 星期六

  // 工作日其他时间不应该匹配
  EXPECT_FALSE(parser.matches(makeTimePoint(2023, 1, 2, 11, 0, 0)));
}

// 测试获取下一个匹配时间点
TEST_F(CronParserTest, GetNextMatchingTime)
{
  // "0 12 * * *" 表示每天中午12点执行
  CronParser parser("0 12 * * *");

  // 从上午11点开始，下一个匹配时间应该是当天中午12点
  auto from = makeTimePoint(2023, 1, 1, 11, 0, 0);
  auto expected = makeTimePoint(2023, 1, 1, 12, 0, 0);
  auto next = parser.getNext(from);

  // 由于时区和夏令时的影响，我们只比较小时和分钟
  std::time_t t1 = std::chrono::system_clock::to_time_t(next);
  std::tm tm1 = *std::localtime(&t1);

  std::time_t t2 = std::chrono::system_clock::to_time_t(expected);
  std::tm tm2 = *std::localtime(&t2);

  EXPECT_EQ(tm1.tm_hour, tm2.tm_hour);
  EXPECT_EQ(tm1.tm_min, tm2.tm_min);

  // 从中午12点1分开始，下一个匹配时间应该是第二天中午12点
  from = makeTimePoint(2023, 1, 1, 12, 1, 0);
  expected = makeTimePoint(2023, 1, 2, 12, 0, 0);
  next = parser.getNext(from);

  t1 = std::chrono::system_clock::to_time_t(next);
  tm1 = *std::localtime(&t1);

  t2 = std::chrono::system_clock::to_time_t(expected);
  tm2 = *std::localtime(&t2);

  EXPECT_EQ(tm1.tm_mday, tm2.tm_mday);
  EXPECT_EQ(tm1.tm_hour, tm2.tm_hour);
  EXPECT_EQ(tm1.tm_min, tm2.tm_min);
}

// 主函数
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}