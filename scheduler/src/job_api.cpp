#include "job_api.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <regex>

namespace scheduler
{

  JobApiHandler::JobApiHandler(JobScheduler &scheduler)
      : scheduler_(scheduler)
  {
    jobDao_ = std::make_unique<JobDAO>();
  }

  std::string JobApiHandler::handleRequest(const std::string &path,
                                           const std::string &method,
                                           const httplib::Params &query_params,
                                           const std::string &content)
  {
    // 使用正则表达式解析路径
    std::regex job_regex("/api/jobs/([^/]+)");
    std::regex job_executions_regex("/api/jobs/([^/]+)/executions");
    std::regex job_execute_regex("/api/jobs/([^/]+)/execute");
    std::smatch matches;

    try
    {
      // 路由请求
      if (path == "/api/jobs" || path == "/api/jobs/")
      {
        if (method == "GET")
        {
          return getJobs(query_params);
        }
        else if (method == "POST")
        {
          return addJob(content);
        }
      }
      else if (std::regex_match(path, matches, job_execute_regex))
      {
        if (method == "POST")
        {
          return executeJob(matches[1].str());
        }
      }
      else if (std::regex_match(path, matches, job_executions_regex))
      {
        if (method == "GET")
        {
          return getJobExecutions(matches[1].str(), query_params);
        }
      }
      else if (std::regex_match(path, matches, job_regex))
      {
        std::string jobId = matches[1].str();
        if (method == "GET")
        {
          return getJob(jobId);
        }
        else if (method == "PUT")
        {
          return updateJob(jobId, content);
        }
        else if (method == "DELETE")
        {
          return deleteJob(jobId);
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
      spdlog::error("处理任务API请求时发生错误: {}", e.what());
      nlohmann::json error;
      error["error"] = e.what();
      error["status"] = 500;
      return error.dump();
    }
  }

  std::string JobApiHandler::getJobs(const httplib::Params &params)
  {
    // 解析分页参数
    int offset = 0;
    int limit = 100;

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

    // 解析类型过滤参数
    JobType type = JobType::ONCE; // 默认值
    bool hasTypeFilter = false;

    auto typeIt = params.find("type");
    if (typeIt != params.end())
    {
      hasTypeFilter = true;
      if (typeIt->second == "PERIODIC")
      {
        type = JobType::PERIODIC;
      }
    }

    // 获取任务列表
    std::vector<JobInfo> jobs;
    if (hasTypeFilter)
    {
      jobs = jobDao_->getJobsByType(type, offset, limit);
    }
    else
    {
      jobs = jobDao_->getAllJobs(offset, limit);
    }

    // 构建响应
    nlohmann::json response = nlohmann::json::array();
    for (const auto &job : jobs)
    {
      response.push_back(job.to_json());
    }

    return response.dump();
  }

  std::string JobApiHandler::getJob(const std::string &jobId)
  {
    // 获取任务详情
    auto job = jobDao_->getJob(jobId);
    if (!job)
    {
      nlohmann::json error;
      error["error"] = "Job not found";
      error["status"] = 404;
      return error.dump();
    }

    return job->to_json().dump();
  }

  std::string JobApiHandler::addJob(const std::string &content)
  {
    try
    {
      // 解析请求内容
      nlohmann::json j = nlohmann::json::parse(content);
      JobInfo job = JobInfo::from_json(j);

      // 提交任务
      std::string jobId = scheduler_.submit_job(job);

      // 构建响应
      nlohmann::json response;
      response["job_id"] = jobId;
      response["status"] = "success";
      response["message"] = "Job submitted successfully";

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

  std::string JobApiHandler::updateJob(const std::string &jobId, const std::string &content)
  {
    try
    {
      // 检查任务是否存在
      auto existingJob = jobDao_->getJob(jobId);
      if (!existingJob)
      {
        nlohmann::json error;
        error["error"] = "Job not found";
        error["status"] = 404;
        return error.dump();
      }

      // 解析请求内容
      nlohmann::json j = nlohmann::json::parse(content);
      JobInfo job = JobInfo::from_json(j);
      job.job_id = jobId; // 确保ID一致

      // 更新任务
      if (!jobDao_->updateJob(job))
      {
        nlohmann::json error;
        error["error"] = "Failed to update job";
        error["status"] = 500;
        return error.dump();
      }

      // 构建响应
      nlohmann::json response;
      response["job_id"] = jobId;
      response["status"] = "success";
      response["message"] = "Job updated successfully";

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

  std::string JobApiHandler::deleteJob(const std::string &jobId)
  {
    // 检查任务是否存在
    auto existingJob = jobDao_->getJob(jobId);
    if (!existingJob)
    {
      nlohmann::json error;
      error["error"] = "Job not found";
      error["status"] = 404;
      return error.dump();
    }

    // 取消任务
    if (!scheduler_.cancel_job(jobId))
    {
      nlohmann::json error;
      error["error"] = "Failed to cancel job";
      error["status"] = 500;
      return error.dump();
    }

    // 删除任务
    if (!jobDao_->deleteJob(jobId))
    {
      nlohmann::json error;
      error["error"] = "Failed to delete job";
      error["status"] = 500;
      return error.dump();
    }

    // 构建响应
    nlohmann::json response;
    response["job_id"] = jobId;
    response["status"] = "success";
    response["message"] = "Job deleted successfully";

    return response.dump();
  }

  std::string JobApiHandler::executeJob(const std::string &jobId)
  {
    // 检查任务是否存在
    auto existingJob = jobDao_->getJob(jobId);
    if (!existingJob)
    {
      nlohmann::json error;
      error["error"] = "Job not found";
      error["status"] = 404;
      return error.dump();
    }

    // 提交任务执行
    std::string newJobId = scheduler_.submit_job(*existingJob);

    // 构建响应
    nlohmann::json response;
    response["job_id"] = jobId;
    response["execution_job_id"] = newJobId;
    response["status"] = "success";
    response["message"] = "Job execution triggered successfully";

    return response.dump();
  }

  std::string JobApiHandler::getJobExecutions(const std::string &jobId, const httplib::Params &params)
  {
    // 检查任务是否存在
    auto existingJob = jobDao_->getJob(jobId);
    if (!existingJob)
    {
      nlohmann::json error;
      error["error"] = "Job not found";
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

    // 获取任务执行记录
    std::vector<JobResult> executions = jobDao_->getJobExecutions(jobId, offset, limit);

    // 构建响应
    nlohmann::json response = nlohmann::json::array();
    for (const auto &execution : executions)
    {
      response.push_back(execution.to_json());
    }

    return response.dump();
  }

} // namespace scheduler