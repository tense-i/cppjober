#pragma once

#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <regex>
#include <stdexcept>

namespace scheduler
{

  class CronParser
  {
  public:
    /**
     * @brief 构造函数
     * @param expression Cron表达式
     * @throw std::invalid_argument 如果表达式格式无效
     */
    explicit CronParser(const std::string &expression);

    /**
     * @brief 检查给定时间是否匹配Cron表达式
     * @param time 要检查的时间点
     * @return 如果时间匹配表达式则返回true
     */
    bool matches(const std::chrono::system_clock::time_point &time) const;

    /**
     * @brief 检查当前时间是否匹配Cron表达式
     * @return 如果当前时间匹配表达式则返回true
     */
    bool matches() const;

    /**
     * @brief 获取下一个匹配的时间点
     * @param from 起始时间点
     * @return 下一个匹配的时间点
     */
    std::chrono::system_clock::time_point getNext(const std::chrono::system_clock::time_point &from) const;

    /**
     * @brief 获取从当前时间开始的下一个匹配时间点
     * @return 下一个匹配的时间点
     */
    std::chrono::system_clock::time_point getNext() const;

    /**
     * @brief 验证Cron表达式是否有效
     * @param expression 要验证的表达式
     * @return 如果表达式有效则返回true
     */
    static bool isValid(const std::string &expression);

  private:
    // 解析Cron表达式
    void parse(const std::string &expression);

    // 解析单个字段
    std::set<int> parseField(const std::string &field, int min, int max);

    // 解析范围表达式 (例如 "1-5")
    std::set<int> parseRange(const std::string &range, int min, int max);

    // 解析步长表达式 (例如 "*/15")
    std::set<int> parseStep(const std::string &step, int min, int max);

    // 检查特定字段是否匹配
    bool fieldMatches(int value, const std::set<int> &fieldSet) const;

    // 存储解析后的Cron表达式字段
    std::set<int> minutes_;     // 分钟 (0-59)
    std::set<int> hours_;       // 小时 (0-23)
    std::set<int> daysOfMonth_; // 日期 (1-31)
    std::set<int> months_;      // 月份 (1-12)
    std::set<int> daysOfWeek_;  // 星期 (0-6, 0=周日)

    // 原始表达式
    std::string expression_;
  };

} // namespace scheduler