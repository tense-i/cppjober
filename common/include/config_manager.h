#pragma once

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <optional>

namespace scheduler
{
  /**
   * @brief 配置管理类，用于管理系统配置
   *
   * 支持从配置文件、环境变量和默认值获取配置
   */
  class ConfigManager
  {
  public:
    /**
     * @brief 获取ConfigManager单例
     * @return ConfigManager单例引用
     */
    static ConfigManager &getInstance();

    /**
     * @brief 从配置文件加载配置
     * @param configFile 配置文件路径
     * @return 是否成功加载
     */
    bool loadFromFile(const std::string &configFile);

    /**
     * @brief 获取配置值
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值，如果不存在则返回默认值
     */
    std::string getString(const std::string &key, const std::string &defaultValue = "") const;

    /**
     * @brief 获取整数配置值
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值，如果不存在或无法转换则返回默认值
     */
    int getInt(const std::string &key, int defaultValue = 0) const;

    /**
     * @brief 获取布尔配置值
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值，如果不存在或无法转换则返回默认值
     */
    bool getBool(const std::string &key, bool defaultValue = false) const;

    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     */
    void set(const std::string &key, const std::string &value);

    /**
     * @brief 获取数据库连接信息
     * @return 包含主机、用户名、密码、数据库名和端口的元组
     */
    std::tuple<std::string, std::string, std::string, std::string, int> getDBConnectionInfo() const;

    /**
     * @brief 获取Kafka连接信息
     * @return Kafka服务器地址
     */
    std::string getKafkaBrokers() const;

  private:
    ConfigManager();
    ~ConfigManager() = default;

    // 禁止拷贝和赋值
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    // 从环境变量加载配置
    void loadFromEnvironment();

    // 获取环境变量值
    std::optional<std::string> getEnv(const std::string &name) const;

    std::map<std::string, std::string> config_;
    mutable std::mutex mutex_;
  };

} // namespace scheduler