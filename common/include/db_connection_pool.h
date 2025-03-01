#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <mysql/mysql.h>

namespace scheduler
{

  class DBConnection
  {
  public:
    DBConnection(const std::string &host,
                 const std::string &user,
                 const std::string &password,
                 const std::string &database,
                 unsigned int port = 3306);
    ~DBConnection();

    bool connect();
    void disconnect();
    bool reconnect();
    bool isConnected() const;

    // 执行SQL查询
    bool executeQuery(const std::string &query);

    // 执行SQL更新（插入、更新、删除）
    bool executeUpdate(const std::string &query);

    // 获取最后一次插入的ID
    uint64_t getLastInsertId();

    // 获取受影响的行数
    uint64_t getAffectedRows();

    // 获取结果集
    MYSQL_RES *getResult();

    // 获取原始MYSQL连接
    MYSQL *getRawConnection() { return mysql_; }

  private:
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    unsigned int port_;
    MYSQL *mysql_;
    bool connected_;
  };

  class DBConnectionPool
  {
  public:
    static DBConnectionPool &getInstance();

    // 使用配置管理器初始化连接池
    void initialize(size_t initialSize = 5, size_t maxSize = 20);

    // 使用指定参数初始化连接池
    void initialize(const std::string &host,
                    const std::string &user,
                    const std::string &password,
                    const std::string &database,
                    unsigned int port = 3306,
                    size_t initialSize = 5,
                    size_t maxSize = 20);

    // 获取连接
    std::shared_ptr<DBConnection> getConnection(unsigned int timeoutMs = 5000);

    // 释放连接
    void releaseConnection(std::shared_ptr<DBConnection> conn);

    // 关闭连接池
    void shutdown();

    // 获取连接池状态
    size_t getActiveConnections() const;
    size_t getIdleConnections() const;

  private:
    DBConnectionPool() = default;
    ~DBConnectionPool();

    // 禁止拷贝和赋值
    DBConnectionPool(const DBConnectionPool &) = delete;
    DBConnectionPool &operator=(const DBConnectionPool &) = delete;

    // 创建新连接
    std::shared_ptr<DBConnection> createConnection();

    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    unsigned int port_;

    size_t initialSize_;
    size_t maxSize_;
    size_t activeConnections_;

    std::queue<std::shared_ptr<DBConnection>> connectionPool_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool shutdown_;
  };

} // namespace scheduler