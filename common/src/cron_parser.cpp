#include "cron_parser.h"
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

namespace scheduler
{

  CronParser::CronParser(const std::string &expression)
      : expression_(expression)
  {
    if (!isValid(expression))
    {
      throw std::invalid_argument("无效的Cron表达式: " + expression);
    }
    parse(expression);
  }

  bool CronParser::matches(const std::chrono::system_clock::time_point &time) const
  {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm *tm = std::localtime(&t);

    return fieldMatches(tm->tm_min, minutes_) &&
           fieldMatches(tm->tm_hour, hours_) &&
           fieldMatches(tm->tm_mday, daysOfMonth_) &&
           fieldMatches(tm->tm_mon + 1, months_) &&                       // tm_mon从0开始，转换为1-12
           fieldMatches(tm->tm_wday == 0 ? 7 : tm->tm_wday, daysOfWeek_); // 将周日(0)转换为7
  }

  bool CronParser::matches() const
  {
    return matches(std::chrono::system_clock::now());
  }

  std::chrono::system_clock::time_point CronParser::getNext(const std::chrono::system_clock::time_point &from) const
  {
    auto next = from + std::chrono::minutes(1);     // 从下一分钟开始检查
    auto end = from + std::chrono::hours(24 * 365); // 最多检查一年

    while (next < end)
    {
      if (matches(next))
      {
        return next;
      }
      next += std::chrono::minutes(1);
    }

    return from; // 如果一年内没有匹配的时间，返回原始时间
  }

  std::chrono::system_clock::time_point CronParser::getNext() const
  {
    return getNext(std::chrono::system_clock::now());
  }

  bool CronParser::isValid(const std::string &expression)
  {
    // 简单验证：检查是否有5个字段
    std::istringstream iss(expression);
    std::string field;
    int count = 0;

    while (iss >> field)
    {
      count++;
    }

    return count == 5;
  }

  void CronParser::parse(const std::string &expression)
  {
    std::istringstream iss(expression);
    std::string field;

    // 解析分钟字段 (0-59)
    iss >> field;
    minutes_ = parseField(field, 0, 59);

    // 解析小时字段 (0-23)
    iss >> field;
    hours_ = parseField(field, 0, 23);

    // 解析日期字段 (1-31)
    iss >> field;
    daysOfMonth_ = parseField(field, 1, 31);

    // 解析月份字段 (1-12)
    iss >> field;
    months_ = parseField(field, 1, 12);

    // 解析星期字段 (0-7, 0和7都表示周日)
    iss >> field;
    daysOfWeek_ = parseField(field, 0, 7);
  }

  std::set<int> CronParser::parseField(const std::string &field, int min, int max)
  {
    // 处理通配符
    if (field == "*")
    {
      std::set<int> values;
      for (int i = min; i <= max; ++i)
      {
        values.insert(i);
      }
      return values;
    }

    // 处理步长
    if (field.find('/') != std::string::npos)
    {
      return parseStep(field, min, max);
    }

    // 处理范围
    if (field.find('-') != std::string::npos)
    {
      return parseRange(field, min, max);
    }

    // 处理列表
    if (field.find(',') != std::string::npos)
    {
      std::set<int> values;
      std::istringstream iss(field);
      std::string item;

      while (std::getline(iss, item, ','))
      {
        int value = std::stoi(item);
        if (value >= min && value <= max)
        {
          values.insert(value);
        }
      }

      return values;
    }

    // 处理单个值
    std::set<int> values;
    int value = std::stoi(field);
    if (value >= min && value <= max)
    {
      values.insert(value);
    }
    return values;
  }

  std::set<int> CronParser::parseRange(const std::string &range, int min, int max)
  {
    std::set<int> values;
    size_t pos = range.find('-');
    if (pos != std::string::npos)
    {
      int start = std::stoi(range.substr(0, pos));
      int end = std::stoi(range.substr(pos + 1));

      for (int i = start; i <= end; ++i)
      {
        if (i >= min && i <= max)
        {
          values.insert(i);
        }
      }
    }
    return values;
  }

  std::set<int> CronParser::parseStep(const std::string &step, int min, int max)
  {
    std::set<int> values;
    size_t pos = step.find('/');
    if (pos != std::string::npos)
    {
      std::string baseRange = step.substr(0, pos);
      int stepValue = std::stoi(step.substr(pos + 1));

      // 处理 */n 形式
      if (baseRange == "*")
      {
        for (int i = min; i <= max; i += stepValue)
        {
          values.insert(i);
        }
      }
      // 处理 a-b/n 形式
      else if (baseRange.find('-') != std::string::npos)
      {
        auto rangeValues = parseRange(baseRange, min, max);
        int count = 0;
        for (int value : rangeValues)
        {
          if (count % stepValue == 0)
          {
            values.insert(value);
          }
          count++;
        }
      }
    }
    return values;
  }

  bool CronParser::fieldMatches(int value, const std::set<int> &fieldSet) const
  {
    return fieldSet.find(value) != fieldSet.end();
  }

} // namespace scheduler