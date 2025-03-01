#include "kafka_message_queue.h"
#include "stats_manager.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <chrono>

namespace scheduler
{

  // 消息传递错误回调类
  class DeliveryReportCb : public RdKafka::DeliveryReportCb
  {
  public:
    void dr_cb(RdKafka::Message &message) override
    {
      if (message.err())
      {
        spdlog::error("Message delivery failed: {}", message.errstr());
      }
      else
      {
        spdlog::debug("Message delivered to topic {}, partition [{}]",
                      message.topic_name(), message.partition());
      }
    }
  };

  // 错误回调类
  class ErrorCb : public RdKafka::EventCb
  {
  public:
    void event_cb(RdKafka::Event &event) override
    {
      switch (event.type())
      {
      case RdKafka::Event::EVENT_ERROR:
        spdlog::error("Kafka error: {}", RdKafka::err2str(event.err()));
        break;
      case RdKafka::Event::EVENT_LOG:
        spdlog::debug("Kafka log: {} ({}): {}", event.fac(), event.severity(), event.str());
        break;
      default:
        spdlog::debug("Kafka event: {}", event.str());
        break;
      }
    }
  };

  // 静态回调实例
  static DeliveryReportCb s_deliveryReportCb;
  static ErrorCb s_errorCb;

  KafkaMessageQueue::KafkaMessageQueue()
      : running_(false)
  {
  }

  KafkaMessageQueue::~KafkaMessageQueue()
  {
    stopConsume();

    // 等待消息队列中的消息发送完成
    if (producer_)
    {
      spdlog::info("Flushing producer...");
      producer_->flush(5000);
    }
  }

  bool KafkaMessageQueue::initProducer(const std::string &brokers)
  {
    std::string errstr;

    // 创建配置
    producerConf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

    // 设置broker列表
    if (producerConf_->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set bootstrap.servers: {}", errstr);
      return false;
    }

    // 设置传递报告回调
    if (producerConf_->set("dr_cb", &s_deliveryReportCb, errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set dr_cb: {}", errstr);
      return false;
    }

    // 设置错误回调
    if (producerConf_->set("event_cb", &s_errorCb, errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set event_cb: {}", errstr);
      return false;
    }

    // 创建生产者
    producer_.reset(RdKafka::Producer::create(producerConf_.get(), errstr));
    if (!producer_)
    {
      spdlog::error("Failed to create producer: {}", errstr);
      return false;
    }

    spdlog::info("Kafka producer initialized, connected to {}", brokers);
    return true;
  }

  bool KafkaMessageQueue::initConsumer(const std::string &brokers,
                                       const std::string &groupId,
                                       const std::vector<std::string> &topics,
                                       MessageCallback callback)
  {
    std::string errstr;

    // 保存主题和回调
    topics_ = topics;
    messageCallback_ = callback;

    // 创建配置
    consumerConf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

    // 设置broker列表
    if (consumerConf_->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set bootstrap.servers: {}", errstr);
      return false;
    }

    // 设置消费者组ID
    if (consumerConf_->set("group.id", groupId, errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set group.id: {}", errstr);
      return false;
    }

    // 设置错误回调
    if (consumerConf_->set("event_cb", &s_errorCb, errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set event_cb: {}", errstr);
      return false;
    }

    // 设置自动提交
    if (consumerConf_->set("enable.auto.commit", "true", errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set enable.auto.commit: {}", errstr);
      return false;
    }

    // 设置自动提交间隔
    if (consumerConf_->set("auto.commit.interval.ms", "5000", errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set auto.commit.interval.ms: {}", errstr);
      return false;
    }

    // 设置自动偏移量重置
    if (consumerConf_->set("auto.offset.reset", "earliest", errstr) != RdKafka::Conf::CONF_OK)
    {
      spdlog::error("Failed to set auto.offset.reset: {}", errstr);
      return false;
    }

    // 创建消费者
    consumer_.reset(RdKafka::KafkaConsumer::create(consumerConf_.get(), errstr));
    if (!consumer_)
    {
      spdlog::error("Failed to create consumer: {}", errstr);
      return false;
    }

    spdlog::info("Kafka consumer initialized, connected to {}, group: {}", brokers, groupId);
    return true;
  }

  bool KafkaMessageQueue::sendMessage(const std::string &topic, const KafkaMessage &message)
  {
    if (!producer_)
    {
      spdlog::error("Producer not initialized");
      return false;
    }

    // 构建消息JSON
    nlohmann::json j;
    j["type"] = messageTypeToString(message.type);
    j["payload"] = message.payload;

    std::string messageStr = j.dump();

    // 发送消息
    RdKafka::ErrorCode err = producer_->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,   // 使用自动分区
        RdKafka::Producer::RK_MSG_COPY, // 复制消息
        const_cast<char *>(messageStr.c_str()),
        messageStr.size(),
        message.key.empty() ? nullptr : message.key.c_str(), // 消息键
        message.key.size(),
        0,      // 不使用时间戳
        nullptr // 不使用消息不透明指针
    );

    if (err != RdKafka::ERR_NO_ERROR)
    {
      spdlog::error("Failed to produce message: {}", RdKafka::err2str(err));
      return false;
    }

    // 轮询以触发传递报告回调
    producer_->poll(0);

    // 更新统计信息
    StatsManager::getInstance().addKafkaMessage(true);

    return true;
  }

  bool KafkaMessageQueue::sendJob(const std::string &topic, const JobInfo &job)
  {
    // 将任务转换为JSON
    nlohmann::json jobJson = job.to_json();
    std::string jobStr = jobJson.dump();

    // 创建消息
    KafkaMessage message(MessageType::JOB_SUBMIT, jobStr, job.job_id);

    // 发送消息
    return sendMessage(topic, message);
  }

  bool KafkaMessageQueue::sendJobResult(const std::string &topic, const JobResult &result)
  {
    // 将结果转换为JSON
    nlohmann::json resultJson = result.to_json();
    std::string resultStr = resultJson.dump();

    // 创建消息
    KafkaMessage message(MessageType::JOB_RESULT, resultStr, result.job_id);

    // 发送消息
    return sendMessage(topic, message);
  }

  bool KafkaMessageQueue::startConsume()
  {
    if (!consumer_)
    {
      spdlog::error("Consumer not initialized");
      return false;
    }

    // 订阅主题
    RdKafka::ErrorCode err = consumer_->subscribe(topics_);
    if (err != RdKafka::ERR_NO_ERROR)
    {
      spdlog::error("Failed to subscribe to topics: {}", RdKafka::err2str(err));
      return false;
    }

    // 启动消费线程
    running_ = true;
    consumeThread_ = std::thread(&KafkaMessageQueue::consumeThread, this);

    spdlog::info("Started consuming from topics: {}",
                 std::accumulate(topics_.begin(), topics_.end(), std::string(),
                                 [](const std::string &a, const std::string &b)
                                 {
                                   return a + (a.empty() ? "" : ", ") + b;
                                 }));

    return true;
  }

  void KafkaMessageQueue::stopConsume()
  {
    if (running_)
    {
      // 设置停止标志
      running_ = false;

      // 通知消费线程
      {
        std::lock_guard<std::mutex> lock(mutex_);
        condition_.notify_all();
      }

      // 等待消费线程结束
      if (consumeThread_.joinable())
      {
        consumeThread_.join();
      }

      // 关闭消费者
      if (consumer_)
      {
        consumer_->close();
      }

      spdlog::info("Stopped consuming messages");
    }
  }

  void KafkaMessageQueue::consumeThread()
  {
    while (running_)
    {
      // 消费消息，超时时间为100ms
      std::unique_ptr<RdKafka::Message> msg(consumer_->consume(100));

      switch (msg->err())
      {
      case RdKafka::ERR_NO_ERROR:
      {
        // 处理消息
        std::string messageStr(static_cast<const char *>(msg->payload()), msg->len());

        try
        {
          // 解析消息JSON
          nlohmann::json j = nlohmann::json::parse(messageStr);

          // 创建消息对象
          KafkaMessage message;
          message.type = stringToMessageType(j["type"].get<std::string>());
          message.payload = j["payload"].get<std::string>();
          message.key = msg->key() ? *msg->key() : "";

          // 更新统计信息
          StatsManager::getInstance().addKafkaMessage(false);

          // 调用回调
          if (messageCallback_)
          {
            messageCallback_(message);
          }
        }
        catch (const std::exception &e)
        {
          spdlog::error("Failed to parse message: {}", e.what());
        }
        break;
      }
      case RdKafka::ERR__PARTITION_EOF:
        // 分区结束，不是错误
        break;
      case RdKafka::ERR__TIMED_OUT:
        // 超时，不是错误
        break;
      default:
        // 其他错误
        spdlog::error("Consumer error: {}", msg->errstr());
        break;
      }

      // 检查是否需要停止
      if (!running_)
      {
        break;
      }
    }
  }

  bool KafkaMessageQueue::isProducerReady() const
  {
    return producer_ != nullptr;
  }

  bool KafkaMessageQueue::isConsumerReady() const
  {
    return consumer_ != nullptr;
  }

  std::string KafkaMessageQueue::messageTypeToString(MessageType type)
  {
    switch (type)
    {
    case MessageType::JOB_SUBMIT:
      return "JOB_SUBMIT";
    case MessageType::JOB_CANCEL:
      return "JOB_CANCEL";
    case MessageType::JOB_RESULT:
      return "JOB_RESULT";
    case MessageType::EXECUTOR_HEARTBEAT:
      return "EXECUTOR_HEARTBEAT";
    default:
      return "UNKNOWN";
    }
  }

  MessageType KafkaMessageQueue::stringToMessageType(const std::string &typeStr)
  {
    if (typeStr == "JOB_SUBMIT")
    {
      return MessageType::JOB_SUBMIT;
    }
    else if (typeStr == "JOB_CANCEL")
    {
      return MessageType::JOB_CANCEL;
    }
    else if (typeStr == "JOB_RESULT")
    {
      return MessageType::JOB_RESULT;
    }
    else if (typeStr == "EXECUTOR_HEARTBEAT")
    {
      return MessageType::EXECUTOR_HEARTBEAT;
    }
    else
    {
      spdlog::warn("Unknown message type: {}", typeStr);
      return MessageType::JOB_SUBMIT; // 默认值
    }
  }

} // namespace scheduler