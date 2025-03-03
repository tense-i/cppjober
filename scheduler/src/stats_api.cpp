#include "stats_api.h"
#include "stats_manager.h"
#include "job_api.h"
#include "executor_api.h"
#include "scheduler.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <thread>
#include <chrono>

namespace scheduler
{

  // StatsApiHandler 实现
  std::string StatsApiHandler::handleRequest(const std::string &path,
                                             const std::string &method,
                                             const httplib::Params &query_params)
  {
    // 只支持GET方法
    if (method != "GET")
    {
      nlohmann::json error;
      error["error"] = "Method not allowed";
      return error.dump();
    }

    // 路由请求
    if (path == "/api/stats" || path == "/api/stats/")
    {
      return getAllStats();
    }
    else if (path == "/api/stats/jobs")
    {
      return getJobStats();
    }
    else if (path == "/api/stats/executors")
    {
      return getExecutorStats();
    }
    else if (path == "/api/stats/system")
    {
      return getSystemStats();
    }
    else if (path == "/api/stats/reset")
    {
      return resetStats();
    }
    else
    {
      nlohmann::json error;
      error["error"] = "Not found";
      return error.dump();
    }
  }

  std::string StatsApiHandler::getAllStats()
  {
    return StatsManager::getInstance().exportStatsAsJson();
  }

  std::string StatsApiHandler::getJobStats()
  {
    nlohmann::json j;
    auto stats = StatsManager::getInstance().getJobStats();

    j["total"] = stats.total_jobs;
    j["pending"] = stats.pending_jobs;
    j["running"] = stats.running_jobs;
    j["completed"] = stats.completed_jobs;
    j["failed"] = stats.failed_jobs;
    j["timeout"] = stats.timeout_jobs;
    j["cancelled"] = stats.cancelled_jobs;
    j["once"] = stats.once_jobs;
    j["periodic"] = stats.periodic_jobs;
    j["avg_execution_time"] = stats.getAvgExecutionTime();
    j["min_execution_time"] = stats.min_execution_time;
    j["max_execution_time"] = stats.max_execution_time;
    j["retry_count"] = stats.retry_count;

    return j.dump(2);
  }

  std::string StatsApiHandler::getExecutorStats()
  {
    nlohmann::json j = nlohmann::json::array();
    auto stats = StatsManager::getInstance().getExecutorStats();

    for (const auto &executor : stats)
    {
      nlohmann::json exec;
      exec["id"] = executor.executor_id;
      exec["address"] = executor.address;
      exec["current_load"] = executor.current_load;
      exec["max_load"] = executor.max_load;
      exec["load_ratio"] = executor.getLoadRatio();
      exec["tasks_executed"] = executor.total_tasks_executed;
      exec["last_heartbeat"] = std::chrono::duration_cast<std::chrono::seconds>(
                                   executor.last_heartbeat.time_since_epoch())
                                   .count();
      exec["online"] = executor.is_online;
      j.push_back(exec);
    }

    return j.dump(2);
  }

  std::string StatsApiHandler::getSystemStats()
  {
    nlohmann::json j;
    auto stats = StatsManager::getInstance().getSystemStats();

    j["uptime"] = stats.scheduler_uptime;
    j["db_query_count"] = stats.db_query_count;
    j["db_query_avg_time"] = stats.getAvgDbQueryTime();
    j["kafka_msg_sent"] = stats.kafka_msg_sent;
    j["kafka_msg_received"] = stats.kafka_msg_received;
    j["scheduler_cycles"] = stats.scheduler_cycles;

    return j.dump(2);
  }

  std::string StatsApiHandler::resetStats()
  {
    StatsManager::getInstance().resetAllStats();

    nlohmann::json j;
    j["status"] = "success";
    j["message"] = "Statistics reset successfully";

    return j.dump(2);
  }

  // ApiServer 实现
  class ApiServer::Impl
  {
  public:
    Impl(int port, JobScheduler &scheduler)
        : port_(port), running_(false),
          jobApiHandler_(scheduler),
          executorApiHandler_()
    {
    }

    void start()
    {
      if (running_)
      {
        return;
      }

      running_ = true;
      server_thread_ = std::thread([this]()
                                   {
        httplib::Server svr;
        
        // 设置统计信息API路由
        svr.Get("/api/stats", [](const httplib::Request& req, httplib::Response& res) {
          res.set_content(StatsApiHandler::handleRequest("/api/stats", "GET", req.params), "application/json");
        });
        
        svr.Get("/api/stats/jobs", [](const httplib::Request& req, httplib::Response& res) {
          res.set_content(StatsApiHandler::handleRequest("/api/stats/jobs", "GET", req.params), "application/json");
        });
        
        svr.Get("/api/stats/executors", [](const httplib::Request& req, httplib::Response& res) {
          res.set_content(StatsApiHandler::handleRequest("/api/stats/executors", "GET", req.params), "application/json");
        });
        
        svr.Get("/api/stats/system", [](const httplib::Request& req, httplib::Response& res) {
          res.set_content(StatsApiHandler::handleRequest("/api/stats/system", "GET", req.params), "application/json");
        });
        
        svr.Get("/api/stats/reset", [](const httplib::Request& req, httplib::Response& res) {
          res.set_content(StatsApiHandler::handleRequest("/api/stats/reset", "GET", req.params), "application/json");
        });
        
        // 设置任务管理API路由
        svr.Get("/api/jobs", [this](const httplib::Request& req, httplib::Response& res) {
          res.set_content(jobApiHandler_.handleRequest("/api/jobs", "GET", req.params, ""), "application/json");
        });
        
        svr.Post("/api/jobs", [this](const httplib::Request& req, httplib::Response& res) {
          res.set_content(jobApiHandler_.handleRequest("/api/jobs", "POST", req.params, req.body), "application/json");
        });
        
        svr.Get(R"(/api/jobs/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/jobs/" + req.matches[1].str();
          res.set_content(jobApiHandler_.handleRequest(path, "GET", req.params, ""), "application/json");
        });
        
        svr.Put(R"(/api/jobs/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/jobs/" + req.matches[1].str();
          res.set_content(jobApiHandler_.handleRequest(path, "PUT", req.params, req.body), "application/json");
        });
        
        svr.Delete(R"(/api/jobs/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/jobs/" + req.matches[1].str();
          res.set_content(jobApiHandler_.handleRequest(path, "DELETE", req.params, ""), "application/json");
        });
        
        svr.Post(R"(/api/jobs/([^/]+)/execute)", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/jobs/" + req.matches[1].str() + "/execute";
          res.set_content(jobApiHandler_.handleRequest(path, "POST", req.params, req.body), "application/json");
        });
        
        svr.Get(R"(/api/jobs/([^/]+)/executions)", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/jobs/" + req.matches[1].str() + "/executions";
          res.set_content(jobApiHandler_.handleRequest(path, "GET", req.params, ""), "application/json");
        });
        
        // 设置执行器管理API路由
        svr.Get("/api/executors", [this](const httplib::Request& req, httplib::Response& res) {
          res.set_content(executorApiHandler_.handleRequest("/api/executors", "GET", req.params, ""), "application/json");
        });
        
        svr.Get(R"(/api/executors/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/executors/" + req.matches[1].str();
          res.set_content(executorApiHandler_.handleRequest(path, "GET", req.params, ""), "application/json");
        });
        
        svr.Put(R"(/api/executors/([^/]+)/load)", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/executors/" + req.matches[1].str() + "/load";
          res.set_content(executorApiHandler_.handleRequest(path, "PUT", req.params, req.body), "application/json");
        });
        
        svr.Put(R"(/api/executors/([^/]+)/status)", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/executors/" + req.matches[1].str() + "/status";
          res.set_content(executorApiHandler_.handleRequest(path, "PUT", req.params, req.body), "application/json");
        });
        
        svr.Get(R"(/api/executors/([^/]+)/tasks)", [this](const httplib::Request& req, httplib::Response& res) {
          std::string path = "/api/executors/" + req.matches[1].str() + "/tasks";
          res.set_content(executorApiHandler_.handleRequest(path, "GET", req.params, ""), "application/json");
        });
        
        // 设置静态文件服务
        svr.set_mount_point("/", "./web");
        
        // 设置CORS头
        svr.set_default_headers({
          {"Access-Control-Allow-Origin", "*"},
          {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
          {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
        });
        
        // 处理OPTIONS请求
        svr.Options(".*", [](const httplib::Request& req, httplib::Response& res) {
          res.set_content("", "text/plain");
        });
        
        // 启动服务器
        spdlog::info("API服务器启动，监听端口: {}", port_);
        svr.listen("0.0.0.0", port_);
        
        spdlog::info("API服务器已停止"); });
    }

    void stop()
    {
      if (!running_)
      {
        return;
      }

      running_ = false;

      if (server_thread_.joinable())
      {
        server_thread_.join();
      }
    }

    bool isRunning() const
    {
      return running_;
    }

  private:
    int port_;
    bool running_;
    std::thread server_thread_;
    JobApiHandler jobApiHandler_;
    ExecutorApiHandler executorApiHandler_;
  };

  ApiServer::ApiServer(int port, JobScheduler &scheduler)
      : impl_(std::make_unique<Impl>(port, scheduler))
  {
  }

  ApiServer::~ApiServer()
  {
    stop();
  }

  void ApiServer::start()
  {
    impl_->start();
  }

  void ApiServer::stop()
  {
    impl_->stop();
  }

  bool ApiServer::isRunning() const
  {
    return impl_->isRunning();
  }

} // namespace scheduler