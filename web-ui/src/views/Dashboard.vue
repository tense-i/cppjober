<template>
  <div class="dashboard-container">
    <!-- 统计卡片 -->
    <el-row :gutter="20">
      <el-col :xs="24" :sm="12" :md="6">
        <div class="stat-card">
          <div class="stat-icon stat-blue">
            <el-icon><Document /></el-icon>
          </div>
          <div class="stat-info">
            <div class="stat-title">总任务数</div>
            <div class="stat-value">{{ stats.jobs.total }}</div>
          </div>
        </div>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <div class="stat-card">
          <div class="stat-icon stat-green">
            <el-icon><Check /></el-icon>
          </div>
          <div class="stat-info">
            <div class="stat-title">成功任务</div>
            <div class="stat-value">{{ stats.jobs.completed }}</div>
          </div>
        </div>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <div class="stat-card">
          <div class="stat-icon stat-orange">
            <el-icon><Loading /></el-icon>
          </div>
          <div class="stat-info">
            <div class="stat-title">运行中任务</div>
            <div class="stat-value">{{ stats.jobs.running }}</div>
          </div>
        </div>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <div class="stat-card">
          <div class="stat-icon stat-red">
            <el-icon><Close /></el-icon>
          </div>
          <div class="stat-info">
            <div class="stat-title">失败任务</div>
            <div class="stat-value">{{ stats.jobs.failed }}</div>
          </div>
        </div>
      </el-col>
    </el-row>

    <!-- 系统信息卡片 -->
    <div class="card-container">
      <div class="card-header">
        <div class="card-title">系统信息</div>
        <el-button type="primary" size="small" @click="refreshStats">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
      </div>
      <el-descriptions :column="2" border>
        <el-descriptions-item label="系统运行时间">{{ formatUptime(stats.system.uptime) }}</el-descriptions-item>
        <el-descriptions-item label="调度周期数">{{ stats.performance.scheduler_cycles }}</el-descriptions-item>
        <el-descriptions-item label="数据库查询次数">{{ stats.performance.db_query_count }}</el-descriptions-item>
        <el-descriptions-item label="平均查询时间">{{ stats.performance.db_query_avg_time }} ms</el-descriptions-item>
        <el-descriptions-item label="Kafka消息发送数">{{ stats.performance.kafka_msg_sent }}</el-descriptions-item>
        <el-descriptions-item label="Kafka消息接收数">{{ stats.performance.kafka_msg_received }}</el-descriptions-item>
      </el-descriptions>
    </div>

    <!-- 任务统计图表 -->
    <el-row :gutter="20">
      <el-col :xs="24" :md="12">
        <div class="card-container">
          <div class="card-header">
            <div class="card-title">任务类型分布</div>
          </div>
          <div class="chart-container" ref="jobTypeChart"></div>
        </div>
      </el-col>
      <el-col :xs="24" :md="12">
        <div class="card-container">
          <div class="card-header">
            <div class="card-title">任务状态分布</div>
          </div>
          <div class="chart-container" ref="jobStatusChart"></div>
        </div>
      </el-col>
    </el-row>

    <!-- 执行器状态 -->
    <div class="card-container">
      <div class="card-header">
        <div class="card-title">执行器状态</div>
      </div>
      <el-table :data="stats.executors" stripe style="width: 100%">
        <el-table-column prop="id" label="执行器ID" width="220" />
        <el-table-column prop="address" label="地址" width="180" />
        <el-table-column label="负载情况" width="200">
          <template #default="scope">
            <el-progress 
              :percentage="Math.round(scope.row.load_ratio * 100)" 
              :status="getLoadStatus(scope.row.load_ratio)"
            />
          </template>
        </el-table-column>
        <el-table-column prop="tasks_executed" label="已执行任务" width="120" />
        <el-table-column label="最后心跳" width="180">
          <template #default="scope">
            {{ formatDate(scope.row.last_heartbeat) }}
          </template>
        </el-table-column>
        <el-table-column label="状态" width="100">
          <template #default="scope">
            <el-tag :type="scope.row.online ? 'success' : 'danger'">
              {{ scope.row.online ? '在线' : '离线' }}
            </el-tag>
          </template>
        </el-table-column>
      </el-table>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onMounted, onUnmounted } from 'vue'
import { Document, Check, Loading, Close, Refresh } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import * as echarts from 'echarts/core'
import { PieChart } from 'echarts/charts'
import { TitleComponent, TooltipComponent, LegendComponent } from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'

// 注册必要的组件
echarts.use([TitleComponent, TooltipComponent, LegendComponent, PieChart, CanvasRenderer])

export default defineComponent({
  name: 'Dashboard',
  components: {
    Document,
    Check,
    Loading,
    Close,
    Refresh
  },
  setup() {
    // 统计数据
    const stats = ref({
      jobs: {
        total: 0,
        pending: 0,
        running: 0,
        completed: 0,
        failed: 0,
        timeout: 0,
        cancelled: 0,
        once: 0,
        periodic: 0
      },
      executors: [] as any[],
      system: {
        uptime: 0,
        timestamp: 0
      },
      performance: {
        db_query_count: 0,
        db_query_avg_time: 0,
        kafka_msg_sent: 0,
        kafka_msg_received: 0,
        scheduler_cycles: 0
      }
    })

    // 图表引用
    const jobTypeChart = ref<HTMLElement | null>(null)
    const jobStatusChart = ref<HTMLElement | null>(null)
    
    // 图表实例
    let jobTypeChartInstance: echarts.ECharts | null = null
    let jobStatusChartInstance: echarts.ECharts | null = null
    
    // 自动刷新定时器
    let refreshTimer: number | null = null

    // 获取统计数据
    const fetchStats = async () => {
      try {
        const response = await fetch('/api/stats')
        if (!response.ok) {
          throw new Error('获取统计数据失败')
        }
        const data = await response.json()
        stats.value = data
        
        // 更新图表
        updateCharts()
      } catch (error) {
        console.error('获取统计数据出错:', error)
        ElMessage.error('获取统计数据失败，请稍后重试')
      }
    }

    // 手动刷新统计数据
    const refreshStats = () => {
      ElMessage.success('正在刷新数据...')
      fetchStats()
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
                { value: stats.value.jobs.once, name: '一次性任务' },
                { value: stats.value.jobs.periodic, name: '周期性任务' }
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
                { value: stats.value.jobs.pending, name: '等待中' },
                { value: stats.value.jobs.running, name: '运行中' },
                { value: stats.value.jobs.completed, name: '已完成' },
                { value: stats.value.jobs.failed, name: '已失败' },
                { value: stats.value.jobs.timeout, name: '已超时' },
                { value: stats.value.jobs.cancelled, name: '已取消' }
              ]
            }
          ]
        })
      }
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
      if (!timestamp) return '未知'
      const date = new Date(timestamp * 1000)
      return date.toLocaleString('zh-CN')
    }

    // 获取负载状态
    const getLoadStatus = (ratio: number) => {
      if (ratio < 0.7) return 'success'
      if (ratio < 0.9) return 'warning'
      return 'exception'
    }

    // 窗口大小变化时重新调整图表大小
    const handleResize = () => {
      jobTypeChartInstance?.resize()
      jobStatusChartInstance?.resize()
    }

    onMounted(() => {
      // 获取初始数据
      fetchStats()
      
      // 初始化图表
      setTimeout(initCharts, 100)
      
      // 设置自动刷新
      refreshTimer = window.setInterval(fetchStats, 30000) // 每30秒刷新一次
      
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
      stats,
      jobTypeChart,
      jobStatusChart,
      refreshStats,
      formatUptime,
      formatDate,
      getLoadStatus
    }
  }
})
</script>

<style scoped>
.dashboard-container {
  padding: 20px;
}

.chart-container {
  height: 300px;
  width: 100%;
}
</style> 