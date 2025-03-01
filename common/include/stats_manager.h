#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include "job.h"
#include "job_dao.h"

namespace scheduler
{
  /**
   * @brief 任务统计信息结构体（内部使用，包含原子类型）
   */
  struct JobStatsAtomic
  {
    // 任务计数
    std::atomic<uint64_t> total_jobs{0};     // 总任务数
    std::atomic<uint64_t> pending_jobs{0};   // 等待中的任务数
    std::atomic<uint64_t> running_jobs{0};   // 运行中的任务数
    std::atomic<uint64_t> completed_jobs{0}; // 已完成的任务数
    std::atomic<uint64_t> failed_jobs{0};    // 失败的任务数
    std::atomic<uint64_t> timeout_jobs{0};   // 超时的任务数
    std::atomic<uint64_t> cancelled_jobs{0}; // 取消的任务数

    // 任务类型统计
    std::atomic<uint64_t> once_jobs{0};     // 一次性任务数
    std::atomic<uint64_t> periodic_jobs{0}; // 周期性任务数

    // 执行时间统计
    std::atomic<uint64_t> total_execution_time{0};        // 总执行时间(毫秒)
    std::atomic<uint64_t> min_execution_time{UINT64_MAX}; // 最小执行时间(毫秒)
    std::atomic<uint64_t> max_execution_time{0};          // 最大执行时间(毫秒)

    // 重试统计
    std::atomic<uint64_t> retry_count{0}; // 重试次数

    // 计算平均执行时间
    uint64_t getAvgExecutionTime() const
    {
      return completed_jobs > 0 ? total_execution_time / completed_jobs : 0;
    }

    // 重置统计信息
    void reset()
    {
      total_jobs = 0;
      pending_jobs = 0;
      running_jobs = 0;
      completed_jobs = 0;
      failed_jobs = 0;
      timeout_jobs = 0;
      cancelled_jobs = 0;
      once_jobs = 0;
      periodic_jobs = 0;
      total_execution_time = 0;
      min_execution_time = UINT64_MAX;
      max_execution_time = 0;
      retry_count = 0;
    }
  };

  /**
   * @brief 任务统计信息结构体（外部使用，不包含原子类型）
   */
  struct JobStats
  {
    // 任务计数
    uint64_t total_jobs{0};     // 总任务数
    uint64_t pending_jobs{0};   // 等待中的任务数
    uint64_t running_jobs{0};   // 运行中的任务数
    uint64_t completed_jobs{0}; // 已完成的任务数
    uint64_t failed_jobs{0};    // 失败的任务数
    uint64_t timeout_jobs{0};   // 超时的任务数
    uint64_t cancelled_jobs{0}; // 取消的任务数

    // 任务类型统计
    uint64_t once_jobs{0};     // 一次性任务数
    uint64_t periodic_jobs{0}; // 周期性任务数

    // 执行时间统计
    uint64_t total_execution_time{0};        // 总执行时间(毫秒)
    uint64_t min_execution_time{UINT64_MAX}; // 最小执行时间(毫秒)
    uint64_t max_execution_time{0};          // 最大执行时间(毫秒)

    // 重试统计
    uint64_t retry_count{0}; // 重试次数

    // 计算平均执行时间
    uint64_t getAvgExecutionTime() const
    {
      return completed_jobs > 0 ? total_execution_time / completed_jobs : 0;
    }
  };

  /**
   * @brief 系统性能统计信息结构体（内部使用，包含原子类型）
   */
  struct SystemStatsAtomic
  {
    std::atomic<uint64_t> scheduler_uptime{0};   // 调度器运行时间(秒)
    std::atomic<uint64_t> db_query_count{0};     // 数据库查询次数
    std::atomic<uint64_t> db_query_time{0};      // 数据库查询总时间(毫秒)
    std::atomic<uint64_t> kafka_msg_sent{0};     // Kafka消息发送数
    std::atomic<uint64_t> kafka_msg_received{0}; // Kafka消息接收数
    std::atomic<uint64_t> scheduler_cycles{0};   // 调度周期数

    // 计算平均数据库查询时间
    uint64_t getAvgDbQueryTime() const
    {
      return db_query_count > 0 ? db_query_time / db_query_count : 0;
    }

    // 重置统计信息
    void reset()
    {
      scheduler_uptime = 0;
      db_query_count = 0;
      db_query_time = 0;
      kafka_msg_sent = 0;
      kafka_msg_received = 0;
      scheduler_cycles = 0;
    }
  };

  /**
   * @brief 系统性能统计信息结构体（外部使用，不包含原子类型）
   */
  struct SystemStats
  {
    uint64_t scheduler_uptime{0};   // 调度器运行时间(秒)
    uint64_t db_query_count{0};     // 数据库查询次数
    uint64_t db_query_time{0};      // 数据库查询总时间(毫秒)
    uint64_t kafka_msg_sent{0};     // Kafka消息发送数
    uint64_t kafka_msg_received{0}; // Kafka消息接收数
    uint64_t scheduler_cycles{0};   // 调度周期数

    // 计算平均数据库查询时间
    uint64_t getAvgDbQueryTime() const
    {
      return db_query_count > 0 ? db_query_time / db_query_count : 0;
    }
  };

  /**
   * @brief 执行器统计信息结构体
   */
  struct ExecutorStats
  {
    std::string executor_id;                              // 执行器ID
    std::string address;                                  // 执行器地址
    uint64_t current_load{0};                             // 当前负载
    uint64_t max_load{0};                                 // 最大负载
    uint64_t total_tasks_executed{0};                     // 已执行任务总数
    std::chrono::system_clock::time_point last_heartbeat; // 最后心跳时间
    bool is_online{false};                                // 是否在线

    // 计算负载比例
    float getLoadRatio() const
    {
      return max_load > 0 ? static_cast<float>(current_load) / max_load : 0.0f;
    }
  };

  /**
   * @brief 统计信息管理类
   */
  class StatsManager
  {
  public:
    /**
     * @brief 获取StatsManager单例
     * @return StatsManager单例引用
     */
    static StatsManager &getInstance();

    /**
     * @brief 更新任务统计信息
     * @param job 任务信息
     * @param status 任务状态
     */
    void updateJobStats(const JobInfo &job, JobStatus status);

    /**
     * @brief 更新任务执行结果统计
     * @param result 任务执行结果
     */
    void updateJobResultStats(const JobResult &result);

    /**
     * @brief 更新执行器统计信息
     * @param executorInfo 执行器信息
     */
    void updateExecutorStats(const ExecutorInfo &executorInfo);

    /**
     * @brief 更新系统性能统计
     * @param metric 指标名称
     * @param value 指标值
     */
    void updateSystemStats(const std::string &metric, uint64_t value);

    /**
     * @brief 增加数据库查询计数和时间
     * @param queryTimeMs 查询时间(毫秒)
     */
    void addDbQuery(uint64_t queryTimeMs);

    /**
     * @brief 增加Kafka消息计数
     * @param sent 是否为发送消息
     */
    void addKafkaMessage(bool sent);

    /**
     * @brief 增加调度周期计数
     */
    void addSchedulerCycle();

    /**
     * @brief 获取任务统计信息
     * @return 任务统计信息
     */
    JobStats getJobStats() const;

    /**
     * @brief 获取执行器统计信息
     * @return 执行器统计信息列表
     */
    std::vector<ExecutorStats> getExecutorStats() const;

    /**
     * @brief 获取系统性能统计信息
     * @return 系统性能统计信息
     */
    SystemStats getSystemStats() const;

    /**
     * @brief 重置所有统计信息
     */
    void resetAllStats();

    /**
     * @brief 生成统计报告
     * @return 统计报告字符串
     */
    std::string generateStatsReport() const;

    /**
     * @brief 导出统计信息为JSON
     * @return JSON字符串
     */
    std::string exportStatsAsJson() const;

    /**
     * @brief 增加取消的任务计数
     */
    void incrementCancelledJobs();

  private:
    StatsManager();
    ~StatsManager() = default;

    // 禁止拷贝和赋值
    StatsManager(const StatsManager &) = delete;
    StatsManager &operator=(const StatsManager &) = delete;

    // 任务统计信息
    JobStatsAtomic jobStats_;

    // 执行器统计信息
    std::map<std::string, ExecutorStats> executorStats_;
    mutable std::mutex executorStatsMutex_;

    // 系统性能统计信息
    SystemStatsAtomic systemStats_;

    // 启动时间
    std::chrono::system_clock::time_point startTime_;
  };

} // namespace scheduler