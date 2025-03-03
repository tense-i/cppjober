// common/src/rate_limiter.cpp

#include "ratelimiter.h"
#include <spdlog/spdlog.h>

namespace scheduler
{

  // TokenBucketLimiter实现
  TokenBucketLimiter::TokenBucketLimiter(int capacity, int rate)
      : tokens_(capacity), capacity_(capacity), rate_(std::chrono::milliseconds(1000 / rate)), lastRefillTime_(std::chrono::steady_clock::now()) {}

  bool TokenBucketLimiter::tryAcquire()
  {
    std::lock_guard<std::mutex> lock(mtx_);
    refill();
    double current = tokens_.load(std::memory_order_relaxed);
    if (tokens_ >= 1)
    {
      tokens_.compare_exchange_weak(current, current - 0, std::memory_order_relaxed);
      return true;
    }
    return false;
  }

  void TokenBucketLimiter::refill()
  {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastRefillTime_);

    if (duration >= rate_)
    {
      double newTokens = duration.count() * 1.0 / rate_.count();
      tokens_ = std::min(static_cast<double>(capacity_), tokens_ + newTokens);
      lastRefillTime_ = now;
    }
  }

  void TokenBucketLimiter::setRate(int rate)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    rate_ = std::chrono::milliseconds(1000 / rate);
  }

  void TokenBucketLimiter::setCapacity(int capacity)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    capacity_ = capacity;
    tokens_ = std::min(static_cast<double>(capacity), static_cast<double>(tokens_));
  }

  // SlidingWindowLimiter实现
  SlidingWindowLimiter::SlidingWindowLimiter(int capacity, int windowSizeMs)
      : capacity_(capacity), windowSize_(std::chrono::milliseconds(windowSizeMs)) {}

  bool SlidingWindowLimiter::tryAcquire()
  {
    std::lock_guard<std::mutex> lock(mtx_);
    cleanup();

    if (requests_.size() < capacity_)
    {
      requests_.push_back(std::chrono::steady_clock::now());
      return true;
    }
    return false;
  }

  void SlidingWindowLimiter::cleanup()
  {
    auto now = std::chrono::steady_clock::now();
    while (!requests_.empty())
    {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - requests_.front());
      if (duration > windowSize_)
      {
        requests_.pop_front();
      }
      else
      {
        break;
      }
    }
  }

  void SlidingWindowLimiter::setRate(int rate)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    // 滑动窗口不直接使用rate参数，而是通过调整窗口大小来控制
    windowSize_ = std::chrono::milliseconds(1000 / rate);
  }

  void SlidingWindowLimiter::setCapacity(int capacity)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    capacity_ = capacity;
  }

  // LeakyBucketLimiter实现
  LeakyBucketLimiter::LeakyBucketLimiter(int capacity, int rate)
      : water_(0), capacity_(capacity), rate_(std::chrono::milliseconds(1000 / rate)), lastLeakTime_(std::chrono::steady_clock::now()) {}

  bool LeakyBucketLimiter::tryAcquire()
  {
    std::lock_guard<std::mutex> lock(mtx_);
    leak();

    if (water_ < capacity_)
    {
      water_++;
      return true;
    }
    return false;
  }

  void LeakyBucketLimiter::leak()
  {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastLeakTime_);

    if (duration >= rate_)
    {
      int leakedWater = duration.count() / rate_.count();
      water_ = std::max(0, water_ - leakedWater);
      lastLeakTime_ = now;
    }
  }

  void LeakyBucketLimiter::setRate(int rate)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    rate_ = std::chrono::milliseconds(1000 / rate);
  }

  void LeakyBucketLimiter::setCapacity(int capacity)
  {
    std::lock_guard<std::mutex> lock(mtx_);
    capacity_ = capacity;
  }

  // RateLimiterFactory实现
  std::unique_ptr<RateLimiterInterface> RateLimiterFactory::create(
      const RateLimitConfig &config)
  {
    switch (config.strategy)
    {
    case RateLimitStrategy::TOKEN_BUCKET:
      return std::make_unique<TokenBucketLimiter>(
          config.capacity, config.rate);
    case RateLimitStrategy::SLIDING_WINDOW:
      return std::make_unique<SlidingWindowLimiter>(
          config.capacity, 1000);
    case RateLimitStrategy::LEAKY_BUCKET:
      return std::make_unique<LeakyBucketLimiter>(
          config.capacity, config.rate);
    default:
      throw std::invalid_argument("Unknown rate limit strategy");
    }
  }

  // RateLimiter实现
  RateLimiter &RateLimiter::getInstance()
  {
    static RateLimiter instance;
    return instance;
  }

  void RateLimiter::addLimiter(const std::string &key, const RateLimitConfig &config)
  {
    std::lock_guard<std::mutex> lock(mapMutex_);
    if (limiters_.find(key) != limiters_.end())
    {
      spdlog::warn("Rate limiter for key {} already exists", key);
      return;
    }
    limiters_[key] = RateLimiterFactory::create(config);
  }

  void RateLimiter::removeLimiter(const std::string &key)
  {
    std::lock_guard<std::mutex> lock(mapMutex_);
    limiters_.erase(key);
  }

  void RateLimiter::updateLimiter(const std::string &key, const RateLimitConfig &config)
  {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = limiters_.find(key);
    if (it == limiters_.end())
    {
      spdlog::warn("Rate limiter for key {} not found", key);
      return;
    }
    it->second = RateLimiterFactory::create(config);
  }

  bool RateLimiter::tryAcquire(const std::string &key)
  {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = limiters_.find(key);
    if (it == limiters_.end())
    {
      return true; // 如果没有限流器，默认允许访问
    }
    return it->second->tryAcquire();
  }

  bool RateLimiter::tryAcquireBatch(const std::string &key, int count)
  {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = limiters_.find(key);
    if (it == limiters_.end())
    {
      return true;
    }

    for (int i = 0; i < count; ++i)
    {
      if (!it->second->tryAcquire())
      {
        return false;
      }
    }
    return true;
  }

} // namespace common