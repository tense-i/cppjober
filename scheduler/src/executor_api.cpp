#include "executor_api.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <regex>

namespace scheduler
{

  ExecutorApiHandler::ExecutorApiHandler()
  {
    jobDao_ = std::make_unique<JobDAO>();
  }

  std::string ExecutorApiHandler::handleRequest(const std::string &path,
                                                const std::string &method,
                                                const httplib::Params &query_params,
                                                const std::string &content)
  {
    // 使用正则表达式解析路径
    std::regex executor_regex("/api/executors/([^/]+)");
    std::regex executor_load_regex("/api/executors/([^/]+)/load");
    std::regex executor_status_regex("/api/executors/([^/]+)/status");
    std::regex executor_tasks_regex("/api/executors/([^/]+)/tasks");
    std::smatch matches;

    try
    {
      // 路由请求
      if (path == "/api/executors" || path == "/api/executors/")
      {
        if (method == "GET")
        {
          return getExecutors();
        }
      }
      else if (std::regex_match(path, matches, executor_load_regex))
      {
        if (method == "PUT")
        {
          return updateExecutorMaxLoad(matches[1].str(), content);
        }
      }
      else if (std::regex_match(path, matches, executor_status_regex))
      {
        if (method == "PUT")
        {
          return updateExecutorStatus(matches[1].str(), content);
        }
      }
      else if (std::regex_match(path, matches, executor_tasks_regex))
      {
        if (method == "GET")
        {
          return getExecutorTasks(matches[1].str(), query_params);
        }
      }
      else if (std::regex_match(path, matches, executor_regex))
      {
        if (method == "GET")
        {
          return getExecutor(matches[1].str());
        }
      }

      // 如果没有匹配的路由，返回404
      nlohmann::json error;
      error["error"] = "Not found";
      error["status"] = 404;
      return error.dump();
    }
    catch (const std::exception &e)
    {
      spdlog::error("处理执行器API请求时发生错误: {}", e.what());
      nlohmann::json error;
      error["error"] = e.what();
      error["status"] = 500;
      return error.dump();
    }
  }

  std::string ExecutorApiHandler::getExecutors()
  {
    // 获取执行器列表
    std::vector<ExecutorInfo> executors = jobDao_->getOnlineExecutorsWithLoad();

    // 构建响应
    nlohmann::json response = nlohmann::json::array();
    for (const auto &executor : executors)
    {
      nlohmann::json executorJson;
      executorJson["executor_id"] = executor.executor_id;
      executorJson["address"] = executor.address;
      executorJson["current_load"] = executor.current_load;
      executorJson["max_load"] = executor.max_load;
      executorJson["total_tasks_executed"] = executor.total_tasks_executed;
      executorJson["last_heartbeat"] = std::chrono::duration_cast<std::chrono::seconds>(
                                           executor.last_heartbeat.time_since_epoch())
                                           .count();
      executorJson["load_ratio"] = static_cast<double>(executor.current_load) / executor.max_load;
      executorJson["online"] = true;

      response.push_back(executorJson);
    }

    return response.dump();
  }

  std::string ExecutorApiHandler::getExecutor(const std::string &executorId)
  {
    // 获取执行器详情
    auto executor = jobDao_->getExecutorInfo(executorId);
    if (!executor)
    {
      nlohmann::json error;
      error["error"] = "Executor not found";
      error["status"] = 404;
      return error.dump();
    }

    // 构建响应
    nlohmann::json response;
    response["executor_id"] = executor->executor_id;
    response["address"] = executor->address;
    response["current_load"] = executor->current_load;
    response["max_load"] = executor->max_load;
    response["total_tasks_executed"] = executor->total_tasks_executed;
    response["last_heartbeat"] = std::chrono::duration_cast<std::chrono::seconds>(
                                     executor->last_heartbeat.time_since_epoch())
                                     .count();
    response["load_ratio"] = static_cast<double>(executor->current_load) / executor->max_load;
    response["online"] = true;

    return response.dump();
  }

  std::string ExecutorApiHandler::updateExecutorMaxLoad(const std::string &executorId, const std::string &content)
  {
    try
    {
      // 检查执行器是否存在
      auto executor = jobDao_->getExecutorInfo(executorId);
      if (!executor)
      {
        nlohmann::json error;
        error["error"] = "Executor not found";
        error["status"] = 404;
        return error.dump();
      }

      // 解析请求内容
      nlohmann::json j = nlohmann::json::parse(content);
      int maxLoad = j.value("max_load", 10);

      // 验证最大负载值
      if (maxLoad <= 0)
      {
        nlohmann::json error;
        error["error"] = "Invalid max load value, must be greater than 0";
        error["status"] = 400;
        return error.dump();
      }

      // 更新执行器最大负载
      if (!jobDao_->updateExecutorMaxLoad(executorId, maxLoad))
      {
        nlohmann::json error;
        error["error"] = "Failed to update executor max load";
        error["status"] = 500;
        return error.dump();
      }

      // 构建响应
      nlohmann::json response;
      response["executor_id"] = executorId;
      response["max_load"] = maxLoad;
      response["status"] = "success";
      response["message"] = "Executor max load updated successfully";

      return response.dump();
    }
    catch (const std::exception &e)
    {
      nlohmann::json error;
      error["error"] = e.what();
      error["status"] = 400;
      return error.dump();
    }
  }

  std::string ExecutorApiHandler::updateExecutorStatus(const std::string &executorId, const std::string &content)
  {
    try
    {
      // 检查执行器是否存在
      auto executor = jobDao_->getExecutorInfo(executorId);
      if (!executor)
      {
        nlohmann::json error;
        error["error"] = "Executor not found";
        error["status"] = 404;
        return error.dump();
      }

      // 解析请求内容
      nlohmann::json j = nlohmann::json::parse(content);
      bool online = j.value("online", true);

      // 更新执行器状态
      if (!jobDao_->updateExecutorStatus(executorId, online))
      {
        nlohmann::json error;
        error["error"] = "Failed to update executor status";
        error["status"] = 500;
        return error.dump();
      }

      // 构建响应
      nlohmann::json response;
      response["executor_id"] = executorId;
      response["online"] = online;
      response["status"] = "success";
      response["message"] = online ? "Executor enabled successfully" : "Executor disabled successfully";

      return response.dump();
    }
    catch (const std::exception &e)
    {
      nlohmann::json error;
      error["error"] = e.what();
      error["status"] = 400;
      return error.dump();
    }
  }

  std::string ExecutorApiHandler::getExecutorTasks(const std::string &executorId, const httplib::Params &params)
  {
    // 检查执行器是否存在
    auto executor = jobDao_->getExecutorInfo(executorId);
    if (!executor)
    {
      nlohmann::json error;
      error["error"] = "Executor not found";
      error["status"] = 404;
      return error.dump();
    }

    // 解析分页参数
    int offset = 0;
    int limit = 10;

    auto offsetIt = params.find("offset");
    if (offsetIt != params.end())
    {
      offset = std::stoi(offsetIt->second);
    }

    auto limitIt = params.find("limit");
    if (limitIt != params.end())
    {
      limit = std::stoi(limitIt->second);
    }

    // 获取执行器任务列表
    // 注意：这里需要在JobDAO中添加一个新方法来获取特定执行器的任务
    // 由于我们没有实现这个方法，这里返回一个空数组
    nlohmann::json response = nlohmann::json::array();

    return response.dump();
  }

} // namespace scheduler