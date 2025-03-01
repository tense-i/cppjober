import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import App from './App.vue'
import router from './router'
import './assets/css/global.css'

// 创建Vue应用实例
const app = createApp(App)

// 使用插件
app.use(ElementPlus, { size: 'default', zIndex: 3000 })
app.use(router)

// 挂载应用
app.mount('#app') 