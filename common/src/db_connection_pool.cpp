#include "db_connection_pool.h"
#include "config_manager.h"
#include "stats_manager.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace scheduler
{

  // DBConnection 实现
  DBConnection::DBConnection(const std::string &host,
                             const std::string &user,
                             const std::string &password,
                             const std::string &database,
                             unsigned int port)
      : host_(host), user_(user), password_(password), database_(database), port_(port),
        mysql_(nullptr), connected_(false)
  {
    mysql_ = mysql_init(nullptr);
  }

  DBConnection::~DBConnection()
  {
    disconnect();
  }

  bool DBConnection::connect()
  {
    if (connected_)
    {
      return true;
    }

    if (!mysql_)
    {
      mysql_ = mysql_init(nullptr);
      if (!mysql_)
      {
        spdlog::error("Failed to initialize MySQL connection");
        return false;
      }
    }

    // 设置连接超时
    int timeout = 5;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // 设置自动重连
    bool reconnect_flag = 1;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect_flag);

    // 建立连接
    if (!mysql_real_connect(mysql_, host_.c_str(), user_.c_str(), password_.c_str(),
                            database_.c_str(), port_, nullptr, 0))
    {
      spdlog::error("Failed to connect to MySQL: {}", mysql_error(mysql_));
      return false;
    }

    // 设置字符集
    mysql_set_character_set(mysql_, "utf8mb4");

    connected_ = true;
    return true;
  }

  void DBConnection::disconnect()
  {
    if (mysql_)
    {
      mysql_close(mysql_);
      mysql_ = nullptr;
    }
    connected_ = false;
  }

  bool DBConnection::reconnect()
  {
    disconnect();
    return connect();
  }

  bool DBConnection::isConnected() const
  {
    return connected_ && (mysql_ping(mysql_) == 0);
  }

  bool DBConnection::executeQuery(const std::string &query)
  {
    if (!isConnected() && !reconnect())
    {
      return false;
    }

    // 记录开始时间
    auto start = std::chrono::high_resolution_clock::now();

    int result = mysql_query(mysql_, query.c_str());

    // 计算查询时间
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 更新统计信息
    StatsManager::getInstance().addDbQuery(duration);

    if (result != 0)
    {
      spdlog::error("Query execution failed: {}", mysql_error(mysql_));
      return false;
    }

    return true;
  }

  bool DBConnection::executeUpdate(const std::string &query)
  {
    return executeQuery(query);
  }

  uint64_t DBConnection::getLastInsertId()
  {
    return mysql_insert_id(mysql_);
  }

  uint64_t DBConnection::getAffectedRows()
  {
    return mysql_affected_rows(mysql_);
  }

  MYSQL_RES *DBConnection::getResult()
  {
    return mysql_store_result(mysql_);
  }

  // DBConnectionPool 实现
  DBConnectionPool &DBConnectionPool::getInstance()
  {
    static DBConnectionPool instance;
    return instance;
  }

  void DBConnectionPool::initialize(size_t initialSize, size_t maxSize)
  {
    // 从配置管理器获取数据库连接信息
    auto [host, user, password, database, port] = ConfigManager::getInstance().getDBConnectionInfo();

    initialize(host, user, password, database, port, initialSize, maxSize);
  }

  void DBConnectionPool::initialize(const std::string &host,
                                    const std::string &user,
                                    const std::string &password,
                                    const std::string &database,
                                    unsigned int port,
                                    size_t initialSize,
                                    size_t maxSize)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;
    port_ = port;
    initialSize_ = initialSize;
    maxSize_ = maxSize;
    activeConnections_ = 0;
    shutdown_ = false;

    // 创建初始连接
    for (size_t i = 0; i < initialSize_; ++i)
    {
      auto conn = createConnection();
      if (conn)
      {
        connectionPool_.push(conn);
      }
    }

    spdlog::info("Database connection pool initialized with {} connections to {}@{}:{}/{}",
                 connectionPool_.size(), user_, host_, port_, database_);
  }

  DBConnectionPool::~DBConnectionPool()
  {
    shutdown();
  }

  std::shared_ptr<DBConnection> DBConnectionPool::getConnection(unsigned int timeoutMs)
  {
    std::unique_lock<std::mutex> lock(mutex_);

    if (shutdown_)
    {
      spdlog::error("Connection pool is shut down");
      return nullptr;
    }

    auto waitUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    // 等待可用连接或超时
    while (connectionPool_.empty())
    {
      // 如果活跃连接数小于最大连接数，创建新连接
      if (activeConnections_ < maxSize_)
      {
        auto conn = createConnection();
        if (conn)
        {
          activeConnections_++;
          return conn;
        }
      }

      // 等待连接释放
      auto status = condition_.wait_until(lock, waitUntil);
      if (status == std::cv_status::timeout)
      {
        spdlog::error("Timeout waiting for database connection");
        return nullptr;
      }

      if (shutdown_)
      {
        spdlog::error("Connection pool was shut down while waiting");
        return nullptr;
      }
    }

    // 获取连接
    auto conn = connectionPool_.front();
    connectionPool_.pop();
    activeConnections_++;

    // 检查连接是否有效，如果无效则重新连接
    if (!conn->isConnected() && !conn->reconnect())
    {
      activeConnections_--;
      return getConnection(timeoutMs);
    }

    return conn;
  }

  void DBConnectionPool::releaseConnection(std::shared_ptr<DBConnection> conn)
  {
    if (!conn)
    {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (!shutdown_)
    {
      connectionPool_.push(conn);
      activeConnections_--;
      condition_.notify_one();
    }
  }

  void DBConnectionPool::shutdown()
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_)
    {
      return;
    }

    shutdown_ = true;

    // 清空连接池
    while (!connectionPool_.empty())
    {
      connectionPool_.pop();
    }

    condition_.notify_all();
    spdlog::info("Database connection pool shut down");
  }

  std::shared_ptr<DBConnection> DBConnectionPool::createConnection()
  {
    auto conn = std::make_shared<DBConnection>(host_, user_, password_, database_, port_);
    if (!conn->connect())
    {
      spdlog::error("Failed to create database connection");
      return nullptr;
    }
    return conn;
  }

  size_t DBConnectionPool::getActiveConnections() const
  {
    return activeConnections_;
  }

  size_t DBConnectionPool::getIdleConnections() const
  {
    return connectionPool_.size();
  }

} // namespace scheduler