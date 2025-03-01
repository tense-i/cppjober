<template>
  <div class="executor-list-container">
    <div class="search-container">
      <el-input
        v-model="searchKeyword"
        placeholder="搜索执行器ID或地址"
        class="search-input"
        clearable
        @keyup.enter="handleSearch"
      >
        <template #prefix>
          <el-icon><Search /></el-icon>
        </template>
      </el-input>
      <el-button type="primary" @click="handleSearch">搜索</el-button>
      <el-button @click="searchKeyword = ''; refreshExecutors()">重置</el-button>
      <el-button type="success" @click="refreshExecutors" :loading="loading">
        <el-icon><Refresh /></el-icon> 刷新
      </el-button>
    </div>

    <!-- 执行器列表 -->
    <div class="card-container">
      <el-table
        v-loading="loading"
        :data="executorList"
        stripe
        style="width: 100%"
        @row-click="handleRowClick"
      >
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
        <el-table-column label="操作" width="220">
          <template #default="{ row }">
            <el-button
              type="primary"
              size="small"
              @click.stop="handleViewExecutor(row)"
            >
              <el-icon><View /></el-icon>
              详情
            </el-button>
            <el-button
              type="warning"
              size="small"
              @click.stop="handleEditMaxLoad(row)"
            >
              <el-icon><Edit /></el-icon>
              设置负载
            </el-button>
            <el-button
              type="danger"
              size="small"
              @click.stop="handleDisableExecutor(row)"
              :disabled="!row.online"
            >
              <el-icon><CircleClose /></el-icon>
              禁用
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

    <!-- 执行器详情对话框 -->
    <el-dialog
      v-model="executorDetailVisible"
      title="执行器详情"
      width="60%"
    >
      <el-descriptions
        title="基本信息"
        :column="2"
        border
      >
        <el-descriptions-item label="执行器ID">{{ currentExecutor.id }}</el-descriptions-item>
        <el-descriptions-item label="执行器地址">{{ currentExecutor.address }}</el-descriptions-item>
        <el-descriptions-item label="当前负载">{{ currentExecutor.current_load }}</el-descriptions-item>
        <el-descriptions-item label="负载比例">
          <el-progress 
            :percentage="Math.round((currentExecutor.current_load / currentExecutor.max_load) * 100)" 
            :status="getLoadStatus(currentExecutor.current_load / currentExecutor.max_load)"
          />
        </el-descriptions-item>
        <el-descriptions-item label="已执行任务数">{{ currentExecutor.tasks_executed }}</el-descriptions-item>
        <el-descriptions-item label="最后心跳时间">{{ formatDate(currentExecutor.last_heartbeat) }}</el-descriptions-item>
        <el-descriptions-item label="状态">
          <el-tag :type="currentExecutor.online ? 'success' : 'danger'">
            {{ currentExecutor.online ? '在线' : '离线' }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="注册时间">{{ formatDate(currentExecutor.register_time) }}</el-descriptions-item>
      </el-descriptions>

      <div class="executor-performance">
        <h3>执行器性能</h3>
        <div ref="executorLoadChart" style="width: 100%; height: 300px;"></div>
      </div>

      <template #footer>
        <span class="dialog-footer">
          <el-button @click="executorDetailVisible = false">关闭</el-button>
          <el-button type="primary" @click="handleViewTasks(currentExecutor)">查看任务</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 设置最大负载对话框 -->
    <el-dialog
      v-model="maxLoadFormVisible"
      title="设置最大负载"
      width="40%"
    >
      <el-form 
        ref="maxLoadFormRef"
        :model="maxLoadForm"
        :rules="maxLoadFormRules"
        label-width="120px"
      >
        <el-form-item label="执行器ID">
          <el-input v-model="maxLoadForm.id" disabled />
        </el-form-item>
        <el-form-item label="当前负载">
          <el-input v-model="maxLoadForm.current_load" disabled />
        </el-form-item>
        <el-form-item label="最大负载" prop="max_load">
          <el-input-number v-model="maxLoadForm.max_load" :min="1" :max="100" />
        </el-form-item>
      </el-form>
      
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="maxLoadFormVisible = false">取消</el-button>
          <el-button type="primary" @click="submitMaxLoadForm">确定</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script lang="ts">
import { ref, reactive, onMounted, defineComponent } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search, Refresh, View, Edit, CircleClose } from '@element-plus/icons-vue'
import * as echarts from 'echarts/core'
import { LineChart } from 'echarts/charts'
import {
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent,
  MarkLineComponent
} from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'
import { executorApi } from '@/api' // 导入API服务

// 注册必要的组件
echarts.use([
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent,
  MarkLineComponent,
  LineChart,
  CanvasRenderer
])

export default defineComponent({
  name: 'ExecutorList',
  components: {
    Search,
    Refresh,
    View,
    Edit,
    CircleClose
  },
  setup() {
    const router = useRouter()
    
    // 执行器列表数据
    const loading = ref(false)
    const executorList = ref<any[]>([])
    const total = ref(0)
    const currentPage = ref(1)
    const pageSize = ref(10)
    const searchKeyword = ref('')
    
    // 当前选中的执行器
    const currentExecutor = ref<any>({})
    
    // 对话框控制
    const executorDetailVisible = ref(false)
    const maxLoadFormVisible = ref(false)
    
    // 图表引用
    const executorLoadChart = ref<HTMLElement | null>(null)
    
    // 图表实例
    let executorLoadChartInstance: echarts.ECharts | null = null
    
    // 最大负载表单
    const maxLoadFormRef = ref()
    const maxLoadForm = reactive({
      id: '',
      current_load: 0,
      max_load: 10
    })
    
    // 表单验证规则
    const maxLoadFormRules = {
      max_load: [
        { required: true, message: '请输入最大负载', trigger: 'blur' },
        { type: 'number', min: 1, max: 100, message: '最大负载必须在1到100之间', trigger: 'blur' }
      ]
    }
    
    // 获取执行器列表
    const fetchExecutors = async () => {
      loading.value = true
      try {
        const data = await executorApi.getExecutors()
        
        // 过滤搜索关键字
        let filteredData = data;
        if (searchKeyword.value) {
          const keyword = searchKeyword.value.toLowerCase();
          filteredData = data.filter((item: any) => 
            item.executor_id.toLowerCase().includes(keyword) || 
            item.address.toLowerCase().includes(keyword)
          );
        }
        
        total.value = filteredData.length
        
        // 分页处理
        const start = (currentPage.value - 1) * pageSize.value
        const end = start + pageSize.value
        executorList.value = filteredData.slice(start, end)
        
      } catch (error) {
        console.error('获取执行器列表失败:', error)
        ElMessage.error('获取执行器列表失败，请稍后重试')
      } finally {
        loading.value = false
      }
    }
    
    // 刷新执行器列表
    const refreshExecutors = () => {
      fetchExecutors()
    }
    
    // 搜索执行器
    const handleSearch = () => {
      currentPage.value = 1
      fetchExecutors()
    }
    
    // 分页大小变化
    const handleSizeChange = (val: number) => {
      pageSize.value = val
      fetchExecutors()
    }
    
    // 当前页变化
    const handleCurrentChange = (val: number) => {
      currentPage.value = val
      fetchExecutors()
    }
    
    // 点击行
    const handleRowClick = (row: any) => {
      handleViewExecutor(row)
    }
    
    // 查看执行器详情
    const handleViewExecutor = async (row: any) => {
      try {
        // 获取执行器详细信息
        const executorDetail = await executorApi.getExecutor(row.executor_id || row.id)
        currentExecutor.value = executorDetail
        executorDetailVisible.value = true
        
        // 初始化图表
        setTimeout(() => {
          initExecutorLoadChart()
        }, 100)
      } catch (error) {
        console.error('获取执行器详情失败:', error)
        ElMessage.error('获取执行器详情失败，请稍后重试')
      }
    }
    
    // 初始化执行器负载图表
    const initExecutorLoadChart = () => {
      if (executorLoadChart.value) {
        if (executorLoadChartInstance) {
          executorLoadChartInstance.dispose()
        }
        
        executorLoadChartInstance = echarts.init(executorLoadChart.value)
        updateExecutorLoadChart()
      }
    }
    
    // 更新执行器负载图表
    const updateExecutorLoadChart = () => {
      if (!executorLoadChartInstance) return
      
      // 模拟历史负载数据
      const now = new Date()
      const times = []
      const loads = []
      
      for (let i = 0; i < 24; i++) {
        const time = new Date(now.getTime() - (23 - i) * 3600 * 1000)
        times.push(time.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' }))
        
        // 生成随机负载数据，但保证最后一个点是当前负载
        let load
        if (i === 23) {
          load = currentExecutor.value.current_load
        } else {
          load = Math.floor(Math.random() * (currentExecutor.value.max_load + 1))
        }
        loads.push(load)
      }
      
      executorLoadChartInstance.setOption({
        title: {
          text: '执行器负载历史',
          left: 'center'
        },
        tooltip: {
          trigger: 'axis',
          formatter: '{b}<br />{a}: {c}'
        },
        xAxis: {
          type: 'category',
          data: times,
          axisLabel: {
            rotate: 45
          }
        },
        yAxis: {
          type: 'value',
          min: 0,
          max: currentExecutor.value.max_load
        },
        series: [
          {
            name: '负载',
            type: 'line',
            data: loads,
            markLine: {
              data: [
                {
                  name: '最大负载',
                  yAxis: currentExecutor.value.max_load
                }
              ]
            }
          }
        ],
        grid: {
          bottom: 80
        }
      })
    }
    
    // 查看执行任务
    const handleViewTasks = (executor: any) => {
      executorDetailVisible.value = false
      router.push(`/job-log?executorId=${executor.executor_id || executor.id}`)
    }
    
    // 编辑最大负载
    const handleEditMaxLoad = (row: any) => {
      maxLoadForm.id = row.executor_id || row.id
      maxLoadForm.current_load = row.current_load
      maxLoadForm.max_load = row.max_load
      maxLoadFormVisible.value = true
    }
    
    // 提交最大负载表单
    const submitMaxLoadForm = async () => {
      maxLoadFormRef.value.validate(async (valid: boolean) => {
        if (valid) {
          try {
            await executorApi.updateExecutorMaxLoad(maxLoadForm.id, maxLoadForm.max_load)
            ElMessage.success('最大负载设置成功')
            maxLoadFormVisible.value = false
            refreshExecutors()
          } catch (error) {
            console.error('设置最大负载失败:', error)
            ElMessage.error('设置最大负载失败，请稍后重试')
          }
        }
      })
    }
    
    // 禁用执行器
    const handleDisableExecutor = (row: any) => {
      ElMessageBox.confirm(
        `确定要禁用执行器 "${row.executor_id || row.id}" 吗？`,
        '禁用确认',
        {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          type: 'warning'
        }
      ).then(async () => {
        try {
          await executorApi.updateExecutorStatus(row.executor_id || row.id, false)
          ElMessage.success('执行器已禁用')
          refreshExecutors()
        } catch (error) {
          console.error('禁用执行器失败:', error)
          ElMessage.error('禁用执行器失败，请稍后重试')
        }
      }).catch(() => {
        // 取消操作
      })
    }
    
    // 获取负载状态
    const getLoadStatus = (ratio: number) => {
      if (ratio >= 0.9) {
        return 'exception'
      } else if (ratio >= 0.7) {
        return 'warning'
      } else {
        return 'success'
      }
    }
    
    // 格式化日期
    const formatDate = (timestamp: number) => {
      if (!timestamp) return '-'
      const date = new Date(timestamp)
      return date.toLocaleString('zh-CN')
    }
    
    onMounted(() => {
      // 设置总数据量
      total.value = 20
      // 获取执行器列表
      fetchExecutors()
    })
    
    return {
      loading,
      executorList,
      total,
      currentPage,
      pageSize,
      searchKeyword,
      currentExecutor,
      executorDetailVisible,
      maxLoadFormVisible,
      maxLoadFormRef,
      maxLoadForm,
      maxLoadFormRules,
      executorLoadChart,
      fetchExecutors,
      refreshExecutors,
      handleSearch,
      handleSizeChange,
      handleCurrentChange,
      handleRowClick,
      handleViewExecutor,
      handleViewTasks,
      handleEditMaxLoad,
      submitMaxLoadForm,
      handleDisableExecutor,
      getLoadStatus,
      formatDate
    }
  }
})
</script>

<style scoped>
.executor-list-container {
  padding: 20px;
}

.search-container {
  margin-bottom: 20px;
  display: flex;
  align-items: center;
}

.search-input {
  width: 300px;
  margin-right: 10px;
}

.pagination-container {
  margin-top: 20px;
  display: flex;
  justify-content: flex-end;
}

.executor-performance {
  margin-top: 20px;
  border: 1px solid #ebeef5;
  border-radius: 4px;
  padding: 20px;
  background-color: #f9f9f9;
}

.executor-performance h3 {
  margin-top: 0;
  margin-bottom: 15px;
  font-size: 16px;
  color: #303133;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
}
</style> 