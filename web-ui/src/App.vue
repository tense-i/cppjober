<template>
  <div class="app-container">
    <el-container>
      <!-- 侧边栏 -->
      <el-aside width="220px">
        <div class="logo">
          <img src="./assets/logo.png" alt="Logo">
          <span>任务调度系统</span>
        </div>
        <el-menu
          :default-active="activeMenu"
          class="el-menu-vertical"
          background-color="#304156"
          text-color="#bfcbd9"
          active-text-color="#409EFF"
          router
          unique-opened
        >
          <el-menu-item index="/">
            <el-icon><HomeFilled /></el-icon>
            <span>首页</span>
          </el-menu-item>
          <el-menu-item index="/dashboard">
            <el-icon><Odometer /></el-icon>
            <span>仪表盘</span>
          </el-menu-item>
          <el-menu-item index="/job-list">
            <el-icon><List /></el-icon>
            <span>任务列表</span>
          </el-menu-item>
          <el-menu-item index="/executor-list">
            <el-icon><Connection /></el-icon>
            <span>执行器列表</span>
          </el-menu-item>
          <el-menu-item index="/job-log">
            <el-icon><Document /></el-icon>
            <span>调度日志</span>
          </el-menu-item>
          <el-menu-item index="/system-info">
            <el-icon><Setting /></el-icon>
            <span>系统信息</span>
          </el-menu-item>
        </el-menu>
      </el-aside>
      
      <!-- 主内容区 -->
      <el-container>
        <!-- 头部 -->
        <el-header>
          <div class="header-left">
            <el-icon class="toggle-sidebar" @click="toggleSidebar"><Fold /></el-icon>
            <el-breadcrumb separator="/">
              <el-breadcrumb-item :to="{ path: '/' }">首页</el-breadcrumb-item>
              <el-breadcrumb-item>{{ currentRoute }}</el-breadcrumb-item>
            </el-breadcrumb>
          </div>
          <div class="header-right">
            <el-dropdown>
              <span class="user-info">
                <img src="./assets/avatar.png" class="user-avatar">
                管理员
                <el-icon><ArrowDown /></el-icon>
              </span>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item>个人信息</el-dropdown-item>
                  <el-dropdown-item>修改密码</el-dropdown-item>
                  <el-dropdown-item divided>退出登录</el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </el-header>
        
        <!-- 内容区 -->
        <el-main>
          <router-view v-slot="{ Component }">
            <transition name="fade" mode="out-in">
              <component :is="Component" />
            </transition>
          </router-view>
        </el-main>
        
        <!-- 页脚 -->
        <el-footer>
          <div class="footer">
            <p>© 2023 分布式任务调度系统 | 版本 v1.0.0</p>
          </div>
        </el-footer>
      </el-container>
    </el-container>
  </div>
</template>

<script lang="ts">
import { defineComponent, computed, ref } from 'vue'
import { useRoute } from 'vue-router'
import { 
  HomeFilled, 
  Odometer, 
  List, 
  Connection, 
  Document, 
  Setting, 
  Fold, 
  ArrowDown 
} from '@element-plus/icons-vue'

export default defineComponent({
  name: 'App',
  components: {
    HomeFilled,
    Odometer,
    List,
    Connection,
    Document,
    Setting,
    Fold,
    ArrowDown
  },
  setup() {
    const route = useRoute()
    const isCollapse = ref(false)
    
    // 当前路由名称
    const currentRoute = computed(() => {
      return route.meta.title || '首页'
    })
    
    // 当前激活的菜单
    const activeMenu = computed(() => {
      return route.path
    })
    
    // 切换侧边栏
    const toggleSidebar = () => {
      isCollapse.value = !isCollapse.value
    }
    
    return {
      currentRoute,
      activeMenu,
      isCollapse,
      toggleSidebar
    }
  }
})
</script>

<style>
/* 全局样式 */
.app-container {
  height: 100vh;
  width: 100%;
}

/* 侧边栏样式 */
.el-aside {
  background-color: #304156;
  color: #fff;
  transition: width 0.3s;
}

.logo {
  height: 60px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: #263445;
  color: #fff;
  font-size: 18px;
  font-weight: bold;
}

.logo img {
  width: 30px;
  margin-right: 10px;
}

.el-menu-vertical {
  border-right: none;
}

/* 头部样式 */
.el-header {
  background-color: #fff;
  color: #333;
  display: flex;
  align-items: center;
  justify-content: space-between;
  box-shadow: 0 1px 4px rgba(0, 21, 41, 0.08);
  padding: 0 20px;
}

.header-left {
  display: flex;
  align-items: center;
}

.toggle-sidebar {
  font-size: 20px;
  margin-right: 15px;
  cursor: pointer;
}

.header-right {
  display: flex;
  align-items: center;
}

.user-info {
  display: flex;
  align-items: center;
  cursor: pointer;
}

.user-avatar {
  width: 30px;
  height: 30px;
  border-radius: 50%;
  margin-right: 8px;
}

/* 内容区样式 */
.el-main {
  background-color: #f0f2f5;
  padding: 20px;
}

/* 页脚样式 */
.el-footer {
  background-color: #fff;
  color: #666;
  text-align: center;
  padding: 15px 0;
  font-size: 12px;
}

/* 过渡动画 */
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style> 