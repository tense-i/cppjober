#include "config_manager.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

namespace scheduler
{

  ConfigManager::ConfigManager()
  {
    // 加载环境变量
    loadFromEnvironment();
  }

  ConfigManager &ConfigManager::getInstance()
  {
    static ConfigManager instance;
    return instance;
  }

  bool ConfigManager::loadFromFile(const std::string &configFile)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream file(configFile);
    if (!file.is_open())
    {
      spdlog::warn("无法打开配置文件: {}", configFile);
      return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
      // 跳过注释和空行
      if (line.empty() || line[0] == '#' || line[0] == ';')
      {
        continue;
      }

      // 解析键值对
      auto pos = line.find('=');
      if (pos != std::string::npos)
      {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // 去除首尾空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 存储配置
        config_[key] = value;
      }
    }

    spdlog::info("成功加载配置文件: {}", configFile);
    return true;
  }

  void ConfigManager::loadFromEnvironment()
  {
    // 数据库配置
    if (auto value = getEnv("DB_HOST"); value)
      config_["db.host"] = *value;
    if (auto value = getEnv("DB_PORT"); value)
      config_["db.port"] = *value;
    if (auto value = getEnv("DB_USER"); value)
      config_["db.user"] = *value;
    if (auto value = getEnv("DB_PASSWORD"); value)
      config_["db.password"] = *value;
    if (auto value = getEnv("DB_NAME"); value)
      config_["db.name"] = *value;

    // Kafka配置
    if (auto value = getEnv("KAFKA_BROKERS"); value)
      config_["kafka.brokers"] = *value;
  }

  std::optional<std::string> ConfigManager::getEnv(const std::string &name) const
  {
    const char *value = std::getenv(name.c_str());
    if (value)
      return std::string(value);
    return std::nullopt;
  }

  std::string ConfigManager::getString(const std::string &key, const std::string &defaultValue) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = config_.find(key);
    if (it != config_.end())
      return it->second;
    return defaultValue;
  }

  int ConfigManager::getInt(const std::string &key, int defaultValue) const
  {
    std::string value = getString(key);
    if (value.empty())
      return defaultValue;

    try
    {
      return std::stoi(value);
    }
    catch (const std::exception &e)
    {
      spdlog::warn("无法将配置值转换为整数: {}={}", key, value);
      return defaultValue;
    }
  }

  bool ConfigManager::getBool(const std::string &key, bool defaultValue) const
  {
    std::string value = getString(key);
    if (value.empty())
      return defaultValue;

    // 转换为小写
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    if (value == "true" || value == "yes" || value == "1" || value == "on")
      return true;
    if (value == "false" || value == "no" || value == "0" || value == "off")
      return false;

    spdlog::warn("无法将配置值转换为布尔值: {}={}", key, value);
    return defaultValue;
  }

  void ConfigManager::set(const std::string &key, const std::string &value)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    config_[key] = value;
  }

  std::tuple<std::string, std::string, std::string, std::string, int> ConfigManager::getDBConnectionInfo() const
  {
    std::string host = getString("db.host", "localhost");
    std::string user = getString("db.user", "root");
    std::string password = getString("db.password", "");
    std::string dbName = getString("db.name", "distributed_scheduler");
    int port = getInt("db.port", 3306);

    return std::make_tuple(host, user, password, dbName, port);
  }

  std::string ConfigManager::getKafkaBrokers() const
  {
    return getString("kafka.brokers", "localhost:9092");
  }

} // namespace scheduler