#pragma once

#include <memory>
#include <string>
#include <functional>
#include <librdkafka/rdkafkacpp.h>
#include "job.h"

namespace scheduler
{

  class KafkaClient
  {
  public:
    using MessageCallback = std::function<void(const std::string &)>;

    KafkaClient(const std::string &brokers,
                const std::string &group_id);
    ~KafkaClient();

    // 生产消息
    bool produce(const std::string &topic, const std::string &message);
    // 消费消息
    void consume(const std::string &topic, MessageCallback callback);
    // 停止消费
    void stop_consume();

    // 发送任务到执行器
    bool send_job(const std::string &executor_id, const JobInfo &job);
    // 发送执行结果到调度器
    bool send_result(const JobResult &result);

  private:
    std::unique_ptr<RdKafka::Producer> producer_;
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
    std::string brokers_;
    std::string group_id_;
    bool running_;
  };

} // namespace scheduler