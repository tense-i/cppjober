#include "cron_parser.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace scheduler;

// 打印时间点
void printTime(const std::chrono::system_clock::time_point &time)
{
  std::time_t t = std::chrono::system_clock::to_time_t(time);
  std::tm *tm = std::localtime(&t);
  std::cout << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << std::endl;
}

int main()
{
  try
  {
    // 测试几个Cron表达式
    std::vector<std::string> expressions = {
        "* * * * *",    // 每分钟
        "0 * * * *",    // 每小时整点
        "0 12 * * *",   // 每天中午12点
        "0 12 * * 1-5", // 工作日中午12点
        "*/15 * * * *", // 每15分钟
        "0 9-17 * * *"  // 每天9点到17点整点
    };

    std::cout << "===== Cron表达式解析器测试 =====" << std::endl;

    for (const auto &expr : expressions)
    {
      std::cout << "\n测试表达式: " << expr << std::endl;

      try
      {
        CronParser parser(expr);

        // 检查当前时间是否匹配
        bool matches = parser.matches();
        std::cout << "当前时间" << (matches ? "匹配" : "不匹配") << std::endl;

        // 获取下一个匹配时间
        auto next = parser.getNext();
        std::cout << "下一个匹配时间: ";
        printTime(next);

        // 获取未来几个匹配时间
        std::cout << "未来5个匹配时间:" << std::endl;
        auto time = std::chrono::system_clock::now();
        for (int i = 0; i < 5; ++i)
        {
          time = parser.getNext(time);
          std::cout << "  " << (i + 1) << ". ";
          printTime(time);
          time += std::chrono::seconds(1); // 加1秒避免重复
        }
      }
      catch (const std::exception &e)
      {
        std::cerr << "错误: " << e.what() << std::endl;
      }
    }

    // 测试无效表达式
    std::cout << "\n测试无效表达式:" << std::endl;
    try
    {
      CronParser parser("invalid");
    }
    catch (const std::exception &e)
    {
      std::cout << "成功捕获异常: " << e.what() << std::endl;
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "未捕获异常: " << e.what() << std::endl;
    return 1;
  }
}