<template>
  <div class="job-log-container">
    <!-- 搜索条件 -->
    <div class="search-container card-container">
      <el-form :inline="true" :model="searchForm" class="search-form">
        <el-form-item label="任务ID">
          <el-input v-model="searchForm.jobId" placeholder="请输入任务ID" clearable />
        </el-form-item>
        <el-form-item label="执行器ID">
          <el-input v-model="searchForm.executorId" placeholder="请输入执行器ID" clearable />
        </el-form-item>
        <el-form-item label="状态">
          <el-select v-model="searchForm.status" placeholder="请选择状态" clearable>
            <el-option label="等待中" value="WAITING" />
            <el-option label="运行中" value="RUNNING" />
            <el-option label="已完成" value="SUCCESS" />
            <el-option label="已失败" value="FAILED" />
            <el-option label="已超时" value="TIMEOUT" />
          </el-select>
        </el-form-item>
        <el-form-item label="时间范围">
          <el-date-picker
            v-model="searchForm.timeRange"
            type="datetimerange"
            range-separator="至"
            start-placeholder="开始时间"
            end-placeholder="结束时间"
            format="YYYY-MM-DD HH:mm:ss"
            value-format="YYYY-MM-DD HH:mm:ss"
          />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">
            <el-icon><Search /></el-icon>查询
          </el-button>
          <el-button @click="resetSearch">
            <el-icon><Refresh /></el-icon>重置
          </el-button>
        </el-form-item>
      </el-form>
    </div>

    <!-- 日志列表 -->
    <div class="card-container">
      <el-table
        v-loading="loading"
        :data="logList"
        stripe
        style="width: 100%"
        @row-click="handleRowClick"
      >
        <el-table-column prop="execution_id" label="执行ID" width="100" />
        <el-table-column prop="job_id" label="任务ID" width="220" />
        <el-table-column prop="job_name" label="任务名称" width="180" />
        <el-table-column prop="executor_id" label="执行器ID" width="220" />
        <el-table-column label="状态" width="100">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">
              {{ getStatusText(scope.row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="开始时间" width="180">
          <template #default="scope">
            {{ formatDate(scope.row.start_time) }}
          </template>
        </el-table-column>
        <el-table-column label="结束时间" width="180">
          <template #default="scope">
            {{ formatDate(scope.row.end_time) }}
          </template>
        </el-table-column>
        <el-table-column label="执行时长" width="120">
          <template #default="scope">
            {{ calculateDuration(scope.row.start_time, scope.row.end_time) }}
          </template>
        </el-table-column>
        <el-table-column label="重试次数" width="100" prop="retry_count" />
        <el-table-column label="操作">
          <template #default="scope">
            <el-button 
              size="small" 
              type="primary" 
              @click.stop="handleViewLog(scope.row)"
            >
              查看详情
            </el-button>
            <el-button 
              size="small" 
              type="success" 
              @click.stop="handleRetryJob(scope.row)"
              :disabled="scope.row.status === 'RUNNING' || scope.row.status === 'WAITING' || scope.row.status === 'SUCCESS'"
            >
              重试
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <!-- 分页 -->
      <div class="pagination-container">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          :total="total"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </div>

    <!-- 日志详情对话框 -->
    <el-dialog
      v-model="logDetailVisible"
      title="执行日志详情"
      width="70%"
    >
      <el-descriptions :column="2" border>
        <el-descriptions-item label="执行ID">{{ currentLog.execution_id }}</el-descriptions-item>
        <el-descriptions-item label="任务ID">{{ currentLog.job_id }}</el-descriptions-item>
        <el-descriptions-item label="任务名称">{{ currentLog.job_name }}</el-descriptions-item>
        <el-descriptions-item label="执行器ID">{{ currentLog.executor_id }}</el-descriptions-item>
        <el-descriptions-item label="状态">
          <el-tag :type="getStatusType(currentLog.status)">
            {{ getStatusText(currentLog.status) }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="重试次数">{{ currentLog.retry_count }}</el-descriptions-item>
        <el-descriptions-item label="开始时间">{{ formatDate(currentLog.start_time) }}</el-descriptions-item>
        <el-descriptions-item label="结束时间">{{ formatDate(currentLog.end_time) }}</el-descriptions-item>
        <el-descriptions-item label="执行时长">{{ calculateDuration(currentLog.start_time, currentLog.end_time) }}</el-descriptions-item>
        <el-descriptions-item label="触发时间">{{ formatDate(currentLog.trigger_time) }}</el-descriptions-item>
      </el-descriptions>

      <div class="log-content">
        <h3>执行命令</h3>
        <el-input
          type="textarea"
          v-model="currentLog.command"
          :rows="3"
          readonly
        />

        <h3>执行输出</h3>
        <div class="log-output">
          <pre>{{ currentLog.output || '无输出' }}</pre>
        </div>

        <h3>错误信息</h3>
        <div class="log-error" v-if="currentLog.error">
          <pre>{{ currentLog.error }}</pre>
        </div>
        <div v-else>无错误</div>
      </div>
      
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="logDetailVisible = false">关闭</el-button>
          <el-button type="primary" @click="handleViewJob">查看任务</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, reactive, onMounted, watch } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search, Refresh } from '@element-plus/icons-vue'
import { jobApi } from '@/api' // 导入API服务

export default defineComponent({
  name: 'JobLog',
  components: {
    Search,
    Refresh
  },
  setup() {
    const router = useRouter()
    const route = useRoute()
    
    // 搜索表单
    const searchForm = reactive({
      jobId: '',
      executorId: '',
      status: '',
      timeRange: [] as string[]
    })
    
    // 日志列表数据
    const loading = ref(false)
    const logList = ref<any[]>([])
    const total = ref(0)
    const currentPage = ref(1)
    const pageSize = ref(10)
    
    // 当前选中的日志
    const currentLog = ref<any>({})
    
    // 对话框控制
    const logDetailVisible = ref(false)
    
    // 从URL参数中获取初始搜索条件
    const initSearchFromQuery = () => {
      if (route.query.jobId) {
        searchForm.jobId = route.query.jobId as string
      }
      
      if (route.query.executorId) {
        searchForm.executorId = route.query.executorId as string
      }
    }
    
    // 获取日志列表
    const fetchLogs = async () => {
      loading.value = true
      try {
        // 构建查询参数
        const params: any = {
          offset: (currentPage.value - 1) * pageSize.value,
          limit: pageSize.value
        }
        
        if (searchForm.jobId) {
          params.jobId = searchForm.jobId
        }
        
        if (searchForm.executorId) {
          params.executorId = searchForm.executorId
        }
        
        if (searchForm.status) {
          params.status = searchForm.status
        }
        
        if (searchForm.timeRange && searchForm.timeRange.length === 2) {
          params.startTime = searchForm.timeRange[0]
          params.endTime = searchForm.timeRange[1]
        }
        
        // 如果有jobId，则获取该任务的执行记录
        if (searchForm.jobId) {
          const data = await jobApi.getJobExecutions(searchForm.jobId, params)
          if (Array.isArray(data)) {
            logList.value = data
            total.value = data.length // 注意：实际应该从后端获取总数
          } else if (data && typeof data === 'object') {
            // 处理可能的分页响应格式
            if (Array.isArray(data.items)) {
              logList.value = data.items
              total.value = data.total || data.items.length
            } else {
              logList.value = []
              total.value = 0
            }
          }
        } else {
          // 如果没有jobId，则获取所有执行记录（这个API可能需要后端支持）
          logList.value = []
          total.value = 0
          ElMessage.warning('请指定任务ID进行查询')
        }
      } catch (error) {
        console.error('获取日志列表失败:', error)
        ElMessage.error('获取日志列表失败，请稍后重试')
        logList.value = []
        total.value = 0
      } finally {
        loading.value = false
      }
    }
    
    // 搜索日志
    const handleSearch = () => {
      currentPage.value = 1
      fetchLogs()
    }
    
    // 重置搜索条件
    const resetSearch = () => {
      searchForm.jobId = ''
      searchForm.executorId = ''
      searchForm.status = ''
      searchForm.timeRange = []
      currentPage.value = 1
      fetchLogs()
    }
    
    // 分页大小变化
    const handleSizeChange = (val: number) => {
      pageSize.value = val
      fetchLogs()
    }
    
    // 当前页变化
    const handleCurrentChange = (val: number) => {
      currentPage.value = val
      fetchLogs()
    }
    
    // 点击行
    const handleRowClick = (row: any) => {
      currentLog.value = row
      logDetailVisible.value = true
    }
    
    // 查看日志详情
    const handleViewLog = (row: any) => {
      currentLog.value = row
      logDetailVisible.value = true
    }
    
    // 查看任务
    const handleViewJob = () => {
      logDetailVisible.value = false
      router.push(`/job-list?jobId=${currentLog.value.job_id}`)
    }
    
    // 重试任务
    const handleRetryJob = (row: any) => {
      ElMessageBox.confirm(
        `确定要重试任务 "${row.job_name}" 吗？`,
        '重试确认',
        {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          type: 'warning'
        }
      ).then(async () => {
        try {
          // 重试任务实际上是重新执行一次任务
          await jobApi.executeJob(row.job_id)
          ElMessage.success(`任务 "${row.job_name}" 已提交重试`)
          fetchLogs()
        } catch (error) {
          console.error('重试任务失败:', error)
          ElMessage.error('重试任务失败，请稍后重试')
        }
      }).catch(() => {
        // 取消操作
      })
    }
    
    // 获取状态类型
    const getStatusType = (status: string) => {
      switch (status) {
        case 'WAITING': return 'info'
        case 'RUNNING': return 'warning'
        case 'SUCCESS': return 'success'
        case 'FAILED': return 'danger'
        case 'TIMEOUT': return 'danger'
        default: return 'info'
      }
    }
    
    // 获取状态文本
    const getStatusText = (status: string) => {
      switch (status) {
        case 'WAITING': return '等待中'
        case 'RUNNING': return '运行中'
        case 'SUCCESS': return '已完成'
        case 'FAILED': return '已失败'
        case 'TIMEOUT': return '已超时'
        default: return '未知'
      }
    }
    
    // 格式化日期
    const formatDate = (timestamp: number) => {
      if (!timestamp) return '-'
      const date = new Date(timestamp)
      return date.toLocaleString('zh-CN')
    }
    
    // 计算执行时长
    const calculateDuration = (startTime: number, endTime: number) => {
      if (!startTime || !endTime) return '-'
      
      const duration = endTime - startTime
      
      if (duration < 0) return '-'
      
      if (duration < 1000) {
        return `${duration}毫秒`
      } else if (duration < 60000) {
        return `${Math.floor(duration / 1000)}秒`
      } else if (duration < 3600000) {
        const minutes = Math.floor(duration / 60000)
        const seconds = Math.floor((duration % 60000) / 1000)
        return `${minutes}分${seconds}秒`
      } else {
        const hours = Math.floor(duration / 3600000)
        const minutes = Math.floor((duration % 3600000) / 60000)
        const seconds = Math.floor((duration % 60000) / 1000)
        return `${hours}时${minutes}分${seconds}秒`
      }
    }
    
    // 监听路由参数变化
    watch(() => route.query, (newQuery) => {
      if (newQuery.jobId !== searchForm.jobId) {
        searchForm.jobId = newQuery.jobId as string || ''
      }
      
      if (newQuery.executorId !== searchForm.executorId) {
        searchForm.executorId = newQuery.executorId as string || ''
      }
      
      if (newQuery.jobId || newQuery.executorId) {
        handleSearch()
      }
    }, { immediate: true })
    
    onMounted(() => {
      // 初始化搜索条件
      initSearchFromQuery()
      
      // 设置总数据量
      total.value = 100
      
      // 获取日志列表
      fetchLogs()
    })
    
    return {
      searchForm,
      loading,
      logList,
      total,
      currentPage,
      pageSize,
      currentLog,
      logDetailVisible,
      handleSearch,
      resetSearch,
      handleSizeChange,
      handleCurrentChange,
      handleRowClick,
      handleViewLog,
      handleViewJob,
      handleRetryJob,
      getStatusType,
      getStatusText,
      formatDate,
      calculateDuration
    }
  }
})
</script>

<style scoped>
.job-log-container {
  padding: 20px;
}

.search-container {
  margin-bottom: 20px;
}

.search-form {
  display: flex;
  flex-wrap: wrap;
}

.pagination-container {
  margin-top: 20px;
  display: flex;
  justify-content: flex-end;
}

.log-content {
  margin-top: 20px;
}

.log-content h3 {
  margin-top: 15px;
  margin-bottom: 10px;
  font-size: 16px;
  font-weight: bold;
}

.log-output, .log-error {
  background-color: #f5f7fa;
  border-radius: 4px;
  padding: 10px;
  max-height: 300px;
  overflow-y: auto;
}

.log-output pre, .log-error pre {
  margin: 0;
  white-space: pre-wrap;
  word-wrap: break-word;
}

.log-error pre {
  color: #f56c6c;
}

@media screen and (max-width: 768px) {
  .search-form .el-form-item {
    margin-right: 0;
    width: 100%;
  }
}
</style> 