import { createRouter, createWebHistory, RouteRecordRaw } from 'vue-router'

// 定义路由
const routes: Array<RouteRecordRaw> = [
  {
    path: '/',
    name: 'Home',
    component: () => import('../views/Home.vue'),
    meta: { title: '首页' }
  },
  {
    path: '/dashboard',
    name: 'Dashboard',
    component: () => import('../views/Dashboard.vue'),
    meta: { title: '仪表盘' }
  },
  {
    path: '/job-list',
    name: 'JobList',
    component: () => import('../views/JobList.vue'),
    meta: { title: '任务列表' }
  },
  {
    path: '/executor-list',
    name: 'ExecutorList',
    component: () => import('../views/ExecutorList.vue'),
    meta: { title: '执行器列表' }
  },
  {
    path: '/job-log',
    name: 'JobLog',
    component: () => import('../views/JobLog.vue'),
    meta: { title: '调度日志' }
  },
  {
    path: '/system-info',
    name: 'SystemInfo',
    component: () => import('../views/SystemInfo.vue'),
    meta: { title: '系统信息' }
  }
]

// 创建路由实例
const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes
})

// 全局前置守卫
router.beforeEach((to, from, next) => {
  // 设置页面标题
  document.title = `${to.meta.title} - 分布式任务调度系统`
  next()
})

export default router 