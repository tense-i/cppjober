<template>
  <div class="system-info-container">
    <!-- 系统概览 -->
    <div class="card-container">
      <div class="card-header">
        <div class="card-title">系统概览</div>
        <el-button type="primary" size="small" @click="refreshSystemInfo">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
      </div>
      
      <el-descriptions :column="2" border>
        <el-descriptions-item label="系统版本">v1.0.0</el-descriptions-item>
        <el-descriptions-item label="运行时间">{{ formatUptime(systemInfo.uptime) }}</el-descriptions-item>
        <el-descriptions-item label="启动时间">{{ formatDate(systemInfo.start_time) }}</el-descriptions-item>
        <el-descriptions-item label="当前时间">{{ formatDate(systemInfo.current_time) }}</el-descriptions-item>
        <el-descriptions-item label="调度周期数">{{ systemInfo.scheduler_cycles }}</el-descriptions-item>
        <el-descriptions-item label="执行器数量">{{ systemInfo.executor_count }}</el-descriptions-item>
      </el-descriptions>
    </div>

    <!-- 性能指标 -->
    <div class="card-container">
      <div class="card-header">
        <div class="card-title">性能指标</div>
      </div>
      
      <el-row :gutter="20">
        <el-col :xs="24" :sm="12">
          <div class="performance-card">
            <h3>数据库</h3>
            <el-descriptions :column="1" border>
              <el-descriptions-item label="查询总数">{{ systemInfo.db_query_count }}</el-descriptions-item>
              <el-descriptions-item label="平均查询时间">{{ systemInfo.db_query_avg_time }} ms</el-descriptions-item>
            </el-descriptions>
          </div>
        </el-col>
        <el-col :xs="24" :sm="12">
          <div class="performance-card">
            <h3>消息队列</h3>
            <el-descriptions :column="1" border>
              <el-descriptions-item label="发送消息数">{{ systemInfo.kafka_msg_sent }}</el-descriptions-item>
              <el-descriptions-item label="接收消息数">{{ systemInfo.kafka_msg_received }}</el-descriptions-item>
            </el-descriptions>
          </div>
        </el-col>
      </el-row>
    </div>

    <!-- 系统配置 -->
    <div class="card-container">
      <div class="card-header">
        <div class="card-title">系统配置</div>
        <el-button type="primary" size="small" @click="handleEditConfig">
          <el-icon><Edit /></el-icon>
          编辑配置
        </el-button>
      </div>
      
      <el-table :data="configList" stripe style="width: 100%">
        <el-table-column prop="key" label="配置项" width="200" />
        <el-table-column prop="value" label="配置值" width="200" />
        <el-table-column prop="description" label="说明" />
        <el-table-column label="更新时间" width="180">
          <template #default="scope">
            {{ formatDate(scope.row.update_time) }}
          </template>
        </el-table-column>
      </el-table>
    </div>

    <!-- 任务统计 -->
    <div class="card-container">
      <div class="card-header">
        <div class="card-title">任务统计</div>
      </div>
      
      <el-row :gutter="20">
        <el-col :xs="24" :md="12">
          <div class="chart-container" ref="jobTypeChart"></div>
        </el-col>
        <el-col :xs="24" :md="12">
          <div class="chart-container" ref="jobStatusChart"></div>
        </el-col>
      </el-row>
    </div>

    <!-- 编辑配置对话框 -->
    <el-dialog
      v-model="configFormVisible"
      title="编辑系统配置"
      width="60%"
    >
      <el-form 
        ref="configFormRef"
        :model="configForm"
        :rules="configFormRules"
        label-width="120px"
      >
        <el-form-item 
          v-for="(item, index) in configForm.items" 
          :key="index"
          :label="item.key"
          :prop="'items.' + index + '.value'"
          :rules="[
            { required: true, message: '请输入配置值', trigger: 'blur' }
          ]"
        >
          <el-input v-model="item.value">
            <template #append>
              <el-tooltip :content="item.description" placement="top">
                <el-icon><InfoFilled /></el-icon>
              </el-tooltip>
            </template>
          </el-input>
        </el-form-item>
      </el-form>
      
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="configFormVisible = false">取消</el-button>
          <el-button type="primary" @click="submitConfigForm">确定</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, reactive, onMounted, onUnmounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Refresh, Edit, InfoFilled } from '@element-plus/icons-vue'
import * as echarts from 'echarts/core'
import { PieChart } from 'echarts/charts'
import { TitleComponent, TooltipComponent, LegendComponent } from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'

// 注册必要的组件
echarts.use([TitleComponent, TooltipComponent, LegendComponent, PieChart, CanvasRenderer])

export default defineComponent({
  name: 'SystemInfo',
  components: {
    Refresh,
    Edit,
    InfoFilled
  },
  setup() {
    // 系统信息
    const systemInfo = ref({
      uptime: 0,
      start_time: 0,
      current_time: Date.now(),
      scheduler_cycles: 0,
      executor_count: 0,
      db_query_count: 0,
      db_query_avg_time: 0,
      kafka_msg_sent: 0,
      kafka_msg_received: 0,
      job_stats: {
        total: 0,
        pending: 0,
        running: 0,
        completed: 0,
        failed: 0,
        timeout: 0,
        cancelled: 0,
        once: 0,
        periodic: 0
      }
    })
    
    // 配置列表
    const configList = ref<any[]>([])
    
    // 图表引用
    const jobTypeChart = ref<HTMLElement | null>(null)
    const jobStatusChart = ref<HTMLElement | null>(null)
    
    // 图表实例
    let jobTypeChartInstance: echarts.ECharts | null = null
    let jobStatusChartInstance: echarts.ECharts | null = null
    
    // 自动刷新定时器
    let refreshTimer: number | null = null
    
    // 配置表单
    const configFormVisible = ref(false)
    const configFormRef = ref()
    const configForm = reactive({
      items: [] as any[]
    })
    
    // 表单验证规则
    const configFormRules = {}
    
    // 获取系统信息
    const fetchSystemInfo = async () => {
      try {
        // 模拟API请求
        setTimeout(() => {
          // 模拟数据
          systemInfo.value = {
            uptime: Math.floor(Math.random() * 86400 * 30), // 最多30天
            start_time: Date.now() - Math.floor(Math.random() * 86400000 * 30),
            current_time: Date.now(),
            scheduler_cycles: Math.floor(Math.random() * 100000),
            executor_count: Math.floor(Math.random() * 10) + 1,
            db_query_count: Math.floor(Math.random() * 10000),
            db_query_avg_time: Math.floor(Math.random() * 100),
            kafka_msg_sent: Math.floor(Math.random() * 5000),
            kafka_msg_received: Math.floor(Math.random() * 3000),
            job_stats: {
              total: 1000,
              pending: 50,
              running: 30,
              completed: 800,
              failed: 80,
              timeout: 20,
              cancelled: 20,
              once: 600,
              periodic: 400
            }
          }
          
          // 更新图表
          updateCharts()
        }, 500)
      } catch (error) {
        console.error('获取系统信息失败:', error)
        ElMessage.error('获取系统信息失败，请稍后重试')
      }
    }
    
    // 获取系统配置
    const fetchSystemConfig = async () => {
      try {
        // 模拟API请求
        setTimeout(() => {
          // 模拟数据
          configList.value = [
            {
              key: 'job_cleanup_days',
              value: '30',
              description: '任务执行记录保留天数',
              update_time: Date.now() - Math.floor(Math.random() * 86400000 * 10)
            },
            {
              key: 'default_timeout',
              value: '60',
              description: '默认任务超时时间（秒）',
              update_time: Date.now() - Math.floor(Math.random() * 86400000 * 10)
            },
            {
              key: 'max_retry_count',
              value: '3',
              description: '最大重试次数',
              update_time: Date.now() - Math.floor(Math.random() * 86400000 * 10)
            },
            {
              key: 'retry_interval',
              value: '30',
              description: '默认重试间隔（秒）',
              update_time: Date.now() - Math.floor(Math.random() * 86400000 * 10)
            },
            {
              key: 'scheduler.check_interval',
              value: '5',
              description: '调度检查间隔（秒）',
              update_time: Date.now() - Math.floor(Math.random() * 86400000 * 10)
            },
            {
              key: 'scheduler.executor_selection_strategy',
              value: 'LEAST_LOAD',
              description: '执行器选择策略',
              update_time: Date.now() - Math.floor(Math.random() * 86400000 * 10)
            }
          ]
        }, 500)
      } catch (error) {
        console.error('获取系统配置失败:', error)
        ElMessage.error('获取系统配置失败，请稍后重试')
      }
    }
    
    // 刷新系统信息
    const refreshSystemInfo = () => {
      fetchSystemInfo()
      ElMessage.success('刷新成功')
    }
    
    // 初始化图表
    const initCharts = () => {
      if (jobTypeChart.value) {
        jobTypeChartInstance = echarts.init(jobTypeChart.value)
      }
      
      if (jobStatusChart.value) {
        jobStatusChartInstance = echarts.init(jobStatusChart.value)
      }
      
      updateCharts()
    }
    
    // 更新图表数据
    const updateCharts = () => {
      // 任务类型图表
      if (jobTypeChartInstance) {
        jobTypeChartInstance.setOption({
          title: {
            text: '任务类型分布',
            left: 'center'
          },
          tooltip: {
            trigger: 'item',
            formatter: '{a} <br/>{b}: {c} ({d}%)'
          },
          legend: {
            orient: 'vertical',
            left: 'left',
            data: ['一次性任务', '周期性任务']
          },
          series: [
            {
              name: '任务类型',
              type: 'pie',
              radius: ['50%', '70%'],
              avoidLabelOverlap: false,
              label: {
                show: false,
                position: 'center'
              },
              emphasis: {
                label: {
                  show: true,
                  fontSize: '18',
                  fontWeight: 'bold'
                }
              },
              labelLine: {
                show: false
              },
              data: [
                { value: systemInfo.value.job_stats.once, name: '一次性任务' },
                { value: systemInfo.value.job_stats.periodic, name: '周期性任务' }
              ]
            }
          ]
        })
      }
      
      // 任务状态图表
      if (jobStatusChartInstance) {
        jobStatusChartInstance.setOption({
          title: {
            text: '任务状态分布',
            left: 'center'
          },
          tooltip: {
            trigger: 'item',
            formatter: '{a} <br/>{b}: {c} ({d}%)'
          },
          legend: {
            orient: 'vertical',
            left: 'left',
            data: ['等待中', '运行中', '已完成', '已失败', '已超时', '已取消']
          },
          series: [
            {
              name: '任务状态',
              type: 'pie',
              radius: ['50%', '70%'],
              avoidLabelOverlap: false,
              label: {
                show: false,
                position: 'center'
              },
              emphasis: {
                label: {
                  show: true,
                  fontSize: '18',
                  fontWeight: 'bold'
                }
              },
              labelLine: {
                show: false
              },
              data: [
                { value: systemInfo.value.job_stats.pending, name: '等待中' },
                { value: systemInfo.value.job_stats.running, name: '运行中' },
                { value: systemInfo.value.job_stats.completed, name: '已完成' },
                { value: systemInfo.value.job_stats.failed, name: '已失败' },
                { value: systemInfo.value.job_stats.timeout, name: '已超时' },
                { value: systemInfo.value.job_stats.cancelled, name: '已取消' }
              ]
            }
          ]
        })
      }
    }
    
    // 编辑配置
    const handleEditConfig = () => {
      configForm.items = configList.value.map(item => ({
        key: item.key,
        value: item.value,
        description: item.description
      }))
      
      configFormVisible.value = true
    }
    
    // 提交配置表单
    const submitConfigForm = () => {
      configFormRef.value.validate((valid: boolean) => {
        if (valid) {
          // 模拟API请求
          setTimeout(() => {
            // 更新配置列表
            configForm.items.forEach(item => {
              const index = configList.value.findIndex(config => config.key === item.key)
              if (index !== -1) {
                configList.value[index].value = item.value
                configList.value[index].update_time = Date.now()
              }
            })
            
            ElMessage.success('配置更新成功')
            configFormVisible.value = false
          }, 500)
        } else {
          return false
        }
      })
    }
    
    // 格式化运行时间
    const formatUptime = (seconds: number) => {
      const days = Math.floor(seconds / 86400)
      const hours = Math.floor((seconds % 86400) / 3600)
      const minutes = Math.floor((seconds % 3600) / 60)
      const secs = seconds % 60
      
      return `${days}天 ${hours}小时 ${minutes}分钟 ${secs}秒`
    }
    
    // 格式化日期
    const formatDate = (timestamp: number) => {
      if (!timestamp) return '-'
      const date = new Date(timestamp)
      return date.toLocaleString('zh-CN')
    }
    
    // 窗口大小变化时重新调整图表大小
    const handleResize = () => {
      jobTypeChartInstance?.resize()
      jobStatusChartInstance?.resize()
    }
    
    onMounted(() => {
      // 获取初始数据
      fetchSystemInfo()
      fetchSystemConfig()
      
      // 初始化图表
      setTimeout(initCharts, 100)
      
      // 设置自动刷新
      refreshTimer = window.setInterval(fetchSystemInfo, 30000) // 每30秒刷新一次
      
      // 监听窗口大小变化
      window.addEventListener('resize', handleResize)
    })
    
    onUnmounted(() => {
      // 清除定时器
      if (refreshTimer) {
        clearInterval(refreshTimer)
      }
      
      // 销毁图表实例
      jobTypeChartInstance?.dispose()
      jobStatusChartInstance?.dispose()
      
      // 移除事件监听
      window.removeEventListener('resize', handleResize)
    })
    
    return {
      systemInfo,
      configList,
      jobTypeChart,
      jobStatusChart,
      configFormVisible,
      configFormRef,
      configForm,
      configFormRules,
      refreshSystemInfo,
      handleEditConfig,
      submitConfigForm,
      formatUptime,
      formatDate
    }
  }
})
</script>

<style scoped>
.system-info-container {
  padding: 20px;
}

.card-container {
  margin-bottom: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.card-title {
  font-size: 16px;
  font-weight: bold;
}

.performance-card {
  margin-bottom: 20px;
}

.performance-card h3 {
  margin-bottom: 10px;
  font-size: 16px;
  font-weight: bold;
}

.chart-container {
  height: 300px;
  width: 100%;
  margin-bottom: 20px;
}
</style> 