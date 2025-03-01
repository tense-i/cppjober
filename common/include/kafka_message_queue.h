#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <librdkafka/rdkafkacpp.h>
#include "job.h"

namespace scheduler
{

  // Kafka消息类型
  enum class MessageType
  {
    JOB_SUBMIT,        // 任务提交
    JOB_CANCEL,        // 任务取消
    JOB_RESULT,        // 任务结果
    EXECUTOR_HEARTBEAT // 执行器心跳
  };

  // Kafka消息
  struct KafkaMessage
  {
    MessageType type;
    std::string payload;
    std::string key;

    KafkaMessage() = default;
    KafkaMessage(MessageType t, const std::string &p, const std::string &k = "")
        : type(t), payload(p), key(k) {}
  };

  // 消息回调接口
  using MessageCallback = std::function<void(const KafkaMessage &)>;

  // Kafka消息队列类
  class KafkaMessageQueue
  {
  public:
    KafkaMessageQueue();
    ~KafkaMessageQueue();

    // 初始化生产者
    bool initProducer(const std::string &brokers);

    // 初始化消费者
    bool initConsumer(const std::string &brokers,
                      const std::string &groupId,
                      const std::vector<std::string> &topics,
                      MessageCallback callback);

    // 发送消息
    bool sendMessage(const std::string &topic, const KafkaMessage &message);

    // 发送任务
    bool sendJob(const std::string &topic, const JobInfo &job);

    // 发送任务结果
    bool sendJobResult(const std::string &topic, const JobResult &result);

    // 开始消费消息
    bool startConsume();

    // 停止消费消息
    void stopConsume();

    // 检查生产者状态
    bool isProducerReady() const;

    // 检查消费者状态
    bool isConsumerReady() const;

  private:
    // 消息类型转换为字符串
    std::string messageTypeToString(MessageType type);

    // 字符串转换为消息类型
    MessageType stringToMessageType(const std::string &typeStr);

    // 消费线程函数
    void consumeThread();

    // Kafka配置
    std::unique_ptr<RdKafka::Conf> producerConf_;
    std::unique_ptr<RdKafka::Conf> consumerConf_;

    // Kafka生产者和消费者
    std::unique_ptr<RdKafka::Producer> producer_;
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;

    // 消费线程
    std::thread consumeThread_;
    std::atomic<bool> running_;

    // 消息回调
    MessageCallback messageCallback_;

    // 消费的主题
    std::vector<std::string> topics_;

    // 互斥锁和条件变量，用于安全停止消费线程
    std::mutex mutex_;
    std::condition_variable condition_;
  };

} // namespace scheduler