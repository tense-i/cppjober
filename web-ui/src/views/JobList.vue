<template>
  <div class="job-list-container">
    <!-- 操作栏 -->
    <div class="action-bar card-container">
      <el-row :gutter="20">
        <el-col :xs="24" :sm="16" :md="18">
          <el-input
            v-model="searchKeyword"
            placeholder="搜索任务名称或ID"
            clearable
            @clear="handleSearch"
            @keyup.enter="handleSearch"
          >
            <template #append>
              <el-button @click="handleSearch">
                <el-icon><Search /></el-icon>
              </el-button>
            </template>
          </el-input>
        </el-col>
        <el-col :xs="24" :sm="8" :md="6" class="action-buttons">
          <el-button type="primary" @click="handleAddJob">
            <el-icon><Plus /></el-icon>添加任务
          </el-button>
          <el-button @click="refreshJobs">
            <el-icon><Refresh /></el-icon>刷新
          </el-button>
        </el-col>
      </el-row>
    </div>

    <!-- 任务列表 -->
    <div class="card-container">
      <el-table
        v-loading="loading"
        :data="jobList"
        stripe
        style="width: 100%"
        @row-click="handleRowClick"
      >
        <el-table-column prop="job_id" label="任务ID" width="220" />
        <el-table-column prop="name" label="任务名称" width="180" />
        <el-table-column label="任务类型" width="120">
          <template #default="scope">
            <el-tag :type="scope.row.type === 'ONCE' ? 'info' : 'success'">
              {{ scope.row.type === 'ONCE' ? '一次性' : '周期性' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="Cron表达式" width="150">
          <template #default="scope">
            <span v-if="scope.row.cron_expression">{{ scope.row.cron_expression }}</span>
            <span v-else>-</span>
          </template>
        </el-table-column>
        <el-table-column prop="priority" label="优先级" width="100" />
        <el-table-column label="状态" width="120">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">
              {{ getStatusText(scope.row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作">
          <template #default="scope">
            <el-button 
              size="small" 
              type="primary" 
              @click.stop="handleViewJob(scope.row)"
            >
              查看
            </el-button>
            <el-button 
              size="small" 
              type="success" 
              @click.stop="handleRunJob(scope.row)"
              :disabled="scope.row.status === 'RUNNING'"
            >
              执行
            </el-button>
            <el-button 
              size="small" 
              type="warning" 
              @click.stop="handleEditJob(scope.row)"
            >
              编辑
            </el-button>
            <el-button 
              size="small" 
              type="danger" 
              @click.stop="handleDeleteJob(scope.row)"
              :disabled="scope.row.status === 'RUNNING'"
            >
              删除
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

    <!-- 任务详情对话框 -->
    <el-dialog
      v-model="jobDetailVisible"
      title="任务详情"
      width="60%"
    >
      <el-descriptions :column="2" border>
        <el-descriptions-item label="任务ID">{{ currentJob.job_id }}</el-descriptions-item>
        <el-descriptions-item label="任务名称">{{ currentJob.name }}</el-descriptions-item>
        <el-descriptions-item label="任务类型">
          <el-tag :type="currentJob.type === 'ONCE' ? 'info' : 'success'">
            {{ currentJob.type === 'ONCE' ? '一次性' : '周期性' }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="优先级">{{ currentJob.priority }}</el-descriptions-item>
        <el-descriptions-item label="Cron表达式" :span="2">
          {{ currentJob.cron_expression || '-' }}
        </el-descriptions-item>
        <el-descriptions-item label="超时时间">{{ currentJob.timeout }} 秒</el-descriptions-item>
        <el-descriptions-item label="重试次数">{{ currentJob.retry_count }}</el-descriptions-item>
        <el-descriptions-item label="重试间隔">{{ currentJob.retry_interval }} 秒</el-descriptions-item>
        <el-descriptions-item label="创建时间">{{ formatDate(currentJob.create_time) }}</el-descriptions-item>
        <el-descriptions-item label="命令" :span="2">
          <el-input
            type="textarea"
            v-model="currentJob.command"
            :rows="4"
            readonly
          />
        </el-descriptions-item>
      </el-descriptions>
      
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="jobDetailVisible = false">关闭</el-button>
          <el-button type="primary" @click="handleViewExecutions">查看执行记录</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 添加/编辑任务对话框 -->
    <el-dialog
      v-model="jobFormVisible"
      :title="isEdit ? '编辑任务' : '添加任务'"
      width="60%"
    >
      <el-form 
        ref="jobFormRef"
        :model="jobForm"
        :rules="jobFormRules"
        label-width="120px"
      >
        <el-form-item label="任务名称" prop="name">
          <el-input v-model="jobForm.name" placeholder="请输入任务名称" />
        </el-form-item>
        <el-form-item label="任务类型" prop="type">
          <el-radio-group v-model="jobForm.type">
            <el-radio label="ONCE">一次性任务</el-radio>
            <el-radio label="PERIODIC">周期性任务</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item 
          label="Cron表达式" 
          prop="cron_expression"
          v-if="jobForm.type === 'PERIODIC'"
        >
          <el-input v-model="jobForm.cron_expression" placeholder="例如: 0 0 * * * (每天0点执行)" />
          <div class="cron-helper">
            <el-link type="primary" @click="showCronHelper = true">Cron表达式帮助</el-link>
          </div>
        </el-form-item>
        <el-form-item label="优先级" prop="priority">
          <el-input-number v-model="jobForm.priority" :min="0" :max="100" />
        </el-form-item>
        <el-form-item label="超时时间(秒)" prop="timeout">
          <el-input-number v-model="jobForm.timeout" :min="1" :max="86400" />
        </el-form-item>
        <el-form-item label="重试次数" prop="retry_count">
          <el-input-number v-model="jobForm.retry_count" :min="0" :max="10" />
        </el-form-item>
        <el-form-item label="重试间隔(秒)" prop="retry_interval">
          <el-input-number v-model="jobForm.retry_interval" :min="0" :max="3600" />
        </el-form-item>
        <el-form-item label="执行命令" prop="command">
          <el-input
            type="textarea"
            v-model="jobForm.command"
            :rows="4"
            placeholder="请输入要执行的命令"
          />
        </el-form-item>
      </el-form>
      
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="jobFormVisible = false">取消</el-button>
          <el-button type="primary" @click="submitJobForm">确定</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- Cron表达式帮助对话框 -->
    <el-dialog
      v-model="showCronHelper"
      title="Cron表达式帮助"
      width="50%"
    >
      <div class="cron-helper-content">
        <p>Cron表达式由5个字段组成，按顺序分别表示：</p>
        <el-table :data="cronFields" border style="width: 100%">
          <el-table-column prop="field" label="字段" width="100" />
          <el-table-column prop="range" label="取值范围" width="150" />
          <el-table-column prop="description" label="说明" />
        </el-table>
        
        <h4 class="mt-20">常用示例：</h4>
        <el-table :data="cronExamples" border style="width: 100%">
          <el-table-column prop="expression" label="表达式" width="150" />
          <el-table-column prop="description" label="说明" />
        </el-table>
      </div>
    </el-dialog>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search, Plus, Refresh } from '@element-plus/icons-vue'
import { jobApi } from '@/api' // 导入API服务

export default defineComponent({
  name: 'JobList',
  components: {
    Search,
    Plus,
    Refresh
  },
  setup() {
    const router = useRouter()
    
    // 任务列表数据
    const loading = ref(false)
    const jobList = ref<any[]>([])
    const total = ref(0)
    const currentPage = ref(1)
    const pageSize = ref(10)
    const searchKeyword = ref('')
    
    // 当前选中的任务
    const currentJob = ref<any>({})
    
    // 对话框控制
    const jobDetailVisible = ref(false)
    const jobFormVisible = ref(false)
    const showCronHelper = ref(false)
    const isEdit = ref(false)
    
    // 任务表单
    const jobFormRef = ref()
    const jobForm = reactive({
      job_id: '',
      name: '',
      type: 'ONCE',
      cron_expression: '',
      priority: 0,
      timeout: 60,
      retry_count: 0,
      retry_interval: 0,
      command: ''
    })
    
    // 表单验证规则
    const jobFormRules = {
      name: [
        { required: true, message: '请输入任务名称', trigger: 'blur' },
        { min: 2, max: 50, message: '长度在 2 到 50 个字符', trigger: 'blur' }
      ],
      type: [
        { required: true, message: '请选择任务类型', trigger: 'change' }
      ],
      cron_expression: [
        { 
          required: true, 
          message: '周期性任务必须设置Cron表达式', 
          trigger: 'blur',
          validator: (rule: any, value: string, callback: any) => {
            if (jobForm.type === 'PERIODIC' && !value) {
              callback(new Error('周期性任务必须设置Cron表达式'))
            } else {
              callback()
            }
          }
        }
      ],
      command: [
        { required: true, message: '请输入执行命令', trigger: 'blur' }
      ]
    }
    
    // Cron表达式帮助数据
    const cronFields = [
      { field: '分钟', range: '0-59', description: '每小时的第几分钟' },
      { field: '小时', range: '0-23', description: '每天的第几小时' },
      { field: '日期', range: '1-31', description: '每月的第几天' },
      { field: '月份', range: '1-12', description: '每年的第几个月' },
      { field: '星期', range: '0-7', description: '每周的第几天，0和7都表示周日' }
    ]
    
    const cronExamples = [
      { expression: '0 0 * * *', description: '每天0点执行' },
      { expression: '*/15 * * * *', description: '每15分钟执行一次' },
      { expression: '0 9-18 * * 1-5', description: '工作日上午9点到下午6点整点执行' },
      { expression: '0 0 1 * *', description: '每月1号0点执行' },
      { expression: '0 12 * * 0', description: '每周日中午12点执行' }
    ]
    
    // 获取任务列表
    const fetchJobs = async () => {
      loading.value = true
      try {
        const params = {
          offset: (currentPage.value - 1) * pageSize.value,
          limit: pageSize.value,
          keyword: searchKeyword.value
        }
        
        const data = await jobApi.getJobs(params)
        
        if (Array.isArray(data)) {
          jobList.value = data
          total.value = data.length // 注意：实际应该从后端获取总数
        } else if (data && typeof data === 'object') {
          // 处理可能的分页响应格式
          if (Array.isArray(data.items)) {
            jobList.value = data.items
            total.value = data.total || data.items.length
          } else {
            jobList.value = []
            total.value = 0
          }
        }
      } catch (error) {
        console.error('获取任务列表失败:', error)
        ElMessage.error('获取任务列表失败，请稍后重试')
        jobList.value = []
        total.value = 0
      } finally {
        loading.value = false
      }
    }
    
    // 刷新任务列表
    const refreshJobs = () => {
      fetchJobs()
    }
    
    // 搜索任务
    const handleSearch = () => {
      currentPage.value = 1
      fetchJobs()
    }
    
    // 分页大小变化
    const handleSizeChange = (val: number) => {
      pageSize.value = val
      fetchJobs()
    }
    
    // 当前页变化
    const handleCurrentChange = (val: number) => {
      currentPage.value = val
      fetchJobs()
    }
    
    // 点击行
    const handleRowClick = (row: any) => {
      handleViewJob(row)
    }
    
    // 查看任务详情
    const handleViewJob = async (row: any) => {
      try {
        const jobDetail = await jobApi.getJob(row.job_id)
        currentJob.value = jobDetail
        jobDetailVisible.value = true
      } catch (error) {
        console.error('获取任务详情失败:', error)
        ElMessage.error('获取任务详情失败，请稍后重试')
      }
    }
    
    // 查看执行记录
    const handleViewExecutions = () => {
      jobDetailVisible.value = false
      router.push(`/job-log?jobId=${currentJob.value.job_id}`)
    }
    
    // 执行任务
    const handleRunJob = (row: any) => {
      ElMessageBox.confirm(
        `确定要执行任务 "${row.name}" 吗？`,
        '执行确认',
        {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          type: 'info'
        }
      ).then(async () => {
        try {
          await jobApi.executeJob(row.job_id)
          ElMessage.success('任务已提交执行')
          refreshJobs()
        } catch (error) {
          console.error('执行任务失败:', error)
          ElMessage.error('执行任务失败，请稍后重试')
        }
      }).catch(() => {
        // 取消操作
      })
    }
    
    // 添加任务
    const handleAddJob = () => {
      isEdit.value = false
      resetJobForm()
      jobFormVisible.value = true
    }
    
    // 编辑任务
    const handleEditJob = async (row: any) => {
      try {
        const jobDetail = await jobApi.getJob(row.job_id)
        isEdit.value = true
        Object.assign(jobForm, jobDetail)
        jobFormVisible.value = true
      } catch (error) {
        console.error('获取任务详情失败:', error)
        ElMessage.error('获取任务详情失败，请稍后重试')
      }
    }
    
    // 删除任务
    const handleDeleteJob = (row: any) => {
      ElMessageBox.confirm(
        `确定要删除任务 "${row.name}" 吗？此操作不可恢复！`,
        '删除确认',
        {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          type: 'warning'
        }
      ).then(async () => {
        try {
          await jobApi.deleteJob(row.job_id)
          ElMessage.success('任务已删除')
          refreshJobs()
        } catch (error) {
          console.error('删除任务失败:', error)
          ElMessage.error('删除任务失败，请稍后重试')
        }
      }).catch(() => {
        // 取消操作
      })
    }
    
    // 提交任务表单
    const submitJobForm = () => {
      jobFormRef.value.validate(async (valid: boolean) => {
        if (valid) {
          try {
            if (isEdit.value) {
              await jobApi.updateJob(jobForm.job_id, jobForm)
              ElMessage.success('任务更新成功')
            } else {
              await jobApi.addJob(jobForm)
              ElMessage.success('任务添加成功')
            }
            jobFormVisible.value = false
            refreshJobs()
          } catch (error) {
            console.error('保存任务失败:', error)
            ElMessage.error('保存任务失败，请稍后重试')
          }
        }
      })
    }
    
    // 重置任务表单
    const resetJobForm = () => {
      jobForm.job_id = ''
      jobForm.name = ''
      jobForm.type = 'ONCE'
      jobForm.cron_expression = ''
      jobForm.priority = 0
      jobForm.timeout = 60
      jobForm.retry_count = 0
      jobForm.retry_interval = 0
      jobForm.command = ''
    }
    
    // 获取状态类型
    const getStatusType = (status: string) => {
      switch (status) {
        case 'WAITING': return 'info'
        case 'RUNNING': return 'warning'
        case 'SUCCESS': return 'success'
        case 'FAILED': return 'danger'
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
        default: return '未知'
      }
    }
    
    // 格式化日期
    const formatDate = (dateStr: string) => {
      if (!dateStr) return '-'
      const date = new Date(dateStr)
      return date.toLocaleString('zh-CN')
    }
    
    onMounted(() => {
      // 设置总数据量
      total.value = 100
      // 获取任务列表
      fetchJobs()
    })
    
    return {
      loading,
      jobList,
      total,
      currentPage,
      pageSize,
      searchKeyword,
      currentJob,
      jobDetailVisible,
      jobFormVisible,
      showCronHelper,
      isEdit,
      jobFormRef,
      jobForm,
      jobFormRules,
      cronFields,
      cronExamples,
      fetchJobs,
      refreshJobs,
      handleSearch,
      handleSizeChange,
      handleCurrentChange,
      handleRowClick,
      handleViewJob,
      handleViewExecutions,
      handleRunJob,
      handleAddJob,
      handleEditJob,
      handleDeleteJob,
      submitJobForm,
      getStatusType,
      getStatusText,
      formatDate
    }
  }
})
</script>

<style scoped>
.job-list-container {
  padding: 20px;
}

.action-bar {
  margin-bottom: 20px;
}

.action-buttons {
  display: flex;
  justify-content: flex-end;
}

.pagination-container {
  margin-top: 20px;
  display: flex;
  justify-content: flex-end;
}

.cron-helper {
  margin-top: 5px;
  font-size: 12px;
}

.cron-helper-content {
  line-height: 1.6;
}

.mt-20 {
  margin-top: 20px;
}

@media screen and (max-width: 768px) {
  .action-buttons {
    margin-top: 10px;
    justify-content: flex-start;
  }
}
</style> 