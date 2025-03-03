// common/include/rate_limiter.h

#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <string>
#include <queue>

namespace scheduler
{

  // 限流策略
  enum class RateLimitStrategy
  {
    TOKEN_BUCKET,   // 令牌桶
    SLIDING_WINDOW, // 滑动窗口
    LEAKY_BUCKET    // 漏桶
  };

  // 限流配置
  struct RateLimitConfig
  {
    int capacity;                     // 容量
    int rate;                         // 速率(每秒)
    RateLimitStrategy strategy;       // 限流策略
    std::chrono::milliseconds window; // 窗口大小
    RateLimitConfig(int cap = 100, int r = 100,
                    RateLimitStrategy s = RateLimitStrategy::TOKEN_BUCKET)
        : capacity(cap), rate(r), strategy(s) {}
  };

  // 限流器接口
  class RateLimiterInterface
  {
  public:
    virtual ~RateLimiterInterface() = default;
    virtual bool tryAcquire() = 0;
    virtual void setRate(int rate) = 0;
    virtual void setCapacity(int capacity) = 0;
  };

  // 令牌桶实现
  class TokenBucketLimiter : public RateLimiterInterface
  {
  public:
    TokenBucketLimiter(int capacity, int rate);
    bool tryAcquire() override;
    void setRate(int rate) override;
    void setCapacity(int capacity) override;

  private:
    void refill();

    std::atomic<double> tokens_;
    int capacity_;
    std::chrono::milliseconds rate_;
    std::chrono::steady_clock::time_point lastRefillTime_;
    std::mutex mtx_;
  };

  // 滑动窗口实现
  class SlidingWindowLimiter : public RateLimiterInterface
  {
  public:
    SlidingWindowLimiter(int capacity, int windowSizeMs = 1000);
    bool tryAcquire() override;
    void setRate(int rate) override;
    void setCapacity(int capacity) override;

  private:
    void cleanup();

    std::deque<std::chrono::steady_clock::time_point> requests_;
    int capacity_;
    std::chrono::milliseconds windowSize_;
    std::mutex mtx_;
  };

  // 漏桶实现
  class LeakyBucketLimiter : public RateLimiterInterface
  {
  public:
    LeakyBucketLimiter(int capacity, int rate);
    bool tryAcquire() override;
    void setRate(int rate) override;
    void setCapacity(int capacity) override;

  private:
    void leak();

    std::atomic<int> water_;
    int capacity_;
    std::chrono::milliseconds rate_;
    std::chrono::steady_clock::time_point lastLeakTime_;
    std::mutex mtx_;
  };

  // 限流器工厂
  class RateLimiterFactory
  {
  public:
    static std::unique_ptr<RateLimiterInterface> create(
        const RateLimitConfig &config);
  };

  // 限流管理器
  class RateLimiter
  {
  public:
    static RateLimiter &getInstance();

    // 添加限流规则
    void addLimiter(const std::string &key, const RateLimitConfig &config);

    // 移除限流规则
    void removeLimiter(const std::string &key);

    // 更新限流规则
    void updateLimiter(const std::string &key, const RateLimitConfig &config);

    // 尝试获取访问权限
    bool tryAcquire(const std::string &key);

    // 批量获取访问权限
    bool tryAcquireBatch(const std::string &key, int count);

  private:
    RateLimiter() = default;
    ~RateLimiter() = default;
    RateLimiter(const RateLimiter &) = delete;
    RateLimiter &operator=(const RateLimiter &) = delete;

    std::unordered_map<std::string, std::unique_ptr<RateLimiterInterface>> limiters_;
    std::mutex mapMutex_;
  };

} // namespace common