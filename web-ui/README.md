# 分布式任务调度系统前端

这是分布式任务调度系统的前端界面，基于Vue 3 + TypeScript + Element Plus开发。

## 功能特性

- 仪表盘：展示系统整体运行状态和统计信息
- 任务管理：支持添加、编辑、删除和执行任务
- 执行器管理：查看和管理执行器节点
- 调度日志：查看任务执行历史和详情
- 系统信息：查看系统配置和性能指标

## 技术栈

- Vue 3：前端框架
- TypeScript：类型系统
- Element Plus：UI组件库
- ECharts：图表可视化
- Axios：HTTP请求
- Vue Router：路由管理

## 开发环境设置

### 前提条件

- Node.js (>= 14.x)
- npm (>= 6.x)

### 安装依赖

```bash
cd web-ui
npm install
```

### 启动开发服务器

```bash
npm run serve
```

### 构建生产版本

```bash
npm run build
```

## 项目结构

```
web-ui/
├── public/              # 静态资源
├── src/
│   ├── assets/          # 资源文件（图片、样式等）
│   ├── components/      # 公共组件
│   ├── views/           # 页面组件
│   ├── router/          # 路由配置
│   ├── App.vue          # 根组件
│   └── main.ts          # 入口文件
├── package.json         # 项目配置
└── tsconfig.json        # TypeScript配置
```

## 与后端API集成

前端通过RESTful API与后端进行通信，主要API端点包括：

- `/api/stats`：获取系统统计信息
- `/api/stats/jobs`：获取任务统计信息
- `/api/stats/executors`：获取执行器统计信息
- `/api/stats/system`：获取系统性能统计信息
- `/api/stats/reset`：重置统计信息

## 浏览器兼容性

支持所有现代浏览器，包括：

- Chrome (最新版)
- Firefox (最新版)
- Safari (最新版)
- Edge (最新版)

## 许可证

MIT 