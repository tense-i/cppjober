<template>
  <div class="home-container">
    <div class="welcome-section">
      <h1>欢迎使用分布式任务调度系统</h1>
      <p>一个高性能、高可用的分布式任务调度平台</p>
      
      <div class="feature-cards">
        <el-row :gutter="20">
          <el-col :xs="24" :sm="12" :md="8" v-for="(feature, index) in features" :key="index">
            <el-card class="feature-card">
              <div class="feature-icon">
                <el-icon>
                  <component :is="feature.icon"></component>
                </el-icon>
              </div>
              <h3>{{ feature.title }}</h3>
              <p>{{ feature.description }}</p>
            </el-card>
          </el-col>
        </el-row>
      </div>
      
      <div class="quick-actions">
        <h2>快速操作</h2>
        <el-row :gutter="20">
          <el-col :xs="24" :sm="8" v-for="(action, index) in quickActions" :key="index">
            <el-button 
              type="primary" 
              class="action-button" 
              @click="navigateTo(action.route)"
              :icon="action.icon"
            >
              {{ action.label }}
            </el-button>
          </el-col>
        </el-row>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue'
import { useRouter } from 'vue-router'
import { 
  Monitor, 
  List, 
  Connection, 
  Document, 
  Setting, 
  Alarm, 
  Share, 
  DataAnalysis, 
  Plus, 
  View, 
  Histogram 
} from '@element-plus/icons-vue'

export default defineComponent({
  name: 'Home',
  components: {
    Monitor,
    List,
    Connection,
    Document,
    Setting,
    Alarm,
    Share,
    DataAnalysis,
    Plus,
    View,
    Histogram
  },
  setup() {
    const router = useRouter()
    
    // 系统特性
    const features = ref([
      {
        icon: 'Alarm',
        title: '定时任务调度',
        description: '支持Cron表达式，灵活配置任务执行时间'
      },
      {
        icon: 'Share',
        title: '分布式架构',
        description: '基于Master-Worker架构，支持水平扩展'
      },
      {
        icon: 'DataAnalysis',
        title: '实时监控',
        description: '任务执行状态实时监控，异常及时告警'
      },
      {
        icon: 'Connection',
        title: '高可用设计',
        description: '支持任务执行失败自动重试，保证任务可靠执行'
      },
      {
        icon: 'Document',
        title: '完整的执行日志',
        description: '记录任务执行全过程，方便问题排查'
      },
      {
        icon: 'Setting',
        title: '可视化配置',
        description: '提供友好的Web界面，方便任务管理和配置'
      }
    ])
    
    // 快速操作
    const quickActions = ref([
      {
        label: '查看仪表盘',
        route: '/dashboard',
        icon: 'Histogram'
      },
      {
        label: '添加新任务',
        route: '/job-list',
        icon: 'Plus'
      },
      {
        label: '查看执行器',
        route: '/executor-list',
        icon: 'View'
      }
    ])
    
    // 导航到指定路由
    const navigateTo = (route: string) => {
      router.push(route)
    }
    
    return {
      features,
      quickActions,
      navigateTo
    }
  }
})
</script>

<style scoped>
.home-container {
  padding: 20px;
}

.welcome-section {
  text-align: center;
  margin-bottom: 40px;
}

.welcome-section h1 {
  font-size: 28px;
  color: #303133;
  margin-bottom: 10px;
}

.welcome-section > p {
  font-size: 16px;
  color: #606266;
  margin-bottom: 40px;
}

.feature-cards {
  margin-top: 40px;
  margin-bottom: 60px;
}

.feature-card {
  height: 100%;
  transition: all 0.3s;
  margin-bottom: 20px;
}

.feature-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 10px 20px rgba(0, 0, 0, 0.1);
}

.feature-icon {
  font-size: 40px;
  color: #409EFF;
  margin-bottom: 15px;
}

.feature-card h3 {
  font-size: 18px;
  margin-bottom: 10px;
  color: #303133;
}

.feature-card p {
  color: #606266;
  font-size: 14px;
}

.quick-actions {
  margin-top: 40px;
}

.quick-actions h2 {
  font-size: 22px;
  color: #303133;
  margin-bottom: 20px;
}

.action-button {
  width: 100%;
  margin-bottom: 15px;
  padding: 15px 0;
  font-size: 16px;
}
</style> 