import axios from 'axios'

// 创建axios实例
const api = axios.create({
  baseURL: '/api',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// 请求拦截器
api.interceptors.request.use(
  config => {
    // 可以在这里添加认证信息等
    return config
  },
  error => {
    console.error('请求错误:', error)
    return Promise.reject(error)
  }
)

// 响应拦截器
api.interceptors.response.use(
  response => {
    return response.data
  },
  error => {
    console.error('响应错误:', error)
    return Promise.reject(error)
  }
)

// 统计信息API
export const statsApi = {
  // 获取所有统计信息
  getAllStats() {
    return api.get('/stats')
  },
  
  // 获取任务统计信息
  getJobStats() {
    return api.get('/stats/jobs')
  },
  
  // 获取执行器统计信息
  getExecutorStats() {
    return api.get('/stats/executors')
  },
  
  // 获取系统性能统计信息
  getSystemStats() {
    return api.get('/stats/system')
  },
  
  // 重置统计信息
  resetStats() {
    return api.get('/stats/reset')
  }
}

// 任务管理API
export const jobApi = {
  // 获取任务列表
  getJobs(params: any) {
    return api.get('/jobs', { params })
  },
  
  // 获取任务详情
  getJob(jobId: string) {
    return api.get(`/jobs/${jobId}`)
  },
  
  // 添加任务
  addJob(job: any) {
    return api.post('/jobs', job)
  },
  
  // 更新任务
  updateJob(jobId: string, job: any) {
    return api.put(`/jobs/${jobId}`, job)
  },
  
  // 删除任务
  deleteJob(jobId: string) {
    return api.delete(`/jobs/${jobId}`)
  },
  
  // 执行任务
  executeJob(jobId: string) {
    return api.post(`/jobs/${jobId}/execute`)
  },
  
  // 获取任务执行记录
  getJobExecutions(jobId: string, params: any) {
    return api.get(`/jobs/${jobId}/executions`, { params })
  }
}

// 执行器管理API
export const executorApi = {
  // 获取执行器列表
  getExecutors() {
    return api.get('/executors')
  },
  
  // 获取执行器详情
  getExecutor(executorId: string) {
    return api.get(`/executors/${executorId}`)
  },
  
  // 更新执行器最大负载
  updateExecutorMaxLoad(executorId: string, maxLoad: number) {
    return api.put(`/executors/${executorId}/load`, { max_load: maxLoad })
  },
  
  // 启用/禁用执行器
  updateExecutorStatus(executorId: string, online: boolean) {
    return api.put(`/executors/${executorId}/status`, { online })
  },
  
  // 获取执行器任务列表
  getExecutorTasks(executorId: string, params: any) {
    return api.get(`/executors/${executorId}/tasks`, { params })
  }
}

export default {
  statsApi,
  jobApi,
  executorApi
} 