# 分布式任务调度系统 - Docker部署

这是一个基于C++17实现的分布式任务调度系统，支持通过Docker进行快速部署和运行。

## 系统架构

系统主要包含以下组件：

1. **调度器(Scheduler)**
   - 负责任务的调度和分发
   - 维护执行器注册信息
   - 处理任务执行结果
   - 提供统计信息API

2. **执行器(Executor)**
   - 接收并执行任务
   - 向调度中心注册
   - 维护心跳
   - 返回执行结果

3. **Web UI**
   - 提供友好的用户界面
   - 展示系统运行状态
   - 管理任务和执行器

4. **基础设施**
   - MySQL: 数据存储
   - Kafka: 消息队列
   - ZooKeeper: 分布式协调

## 功能特性

1. **任务管理**
   - 支持一次性任务和周期性任务
   - 基于优先级的任务调度
   - 任务超时控制
   - 失败重试机制

2. **执行器管理**
   - 自动注册和发现
   - 心跳检测
   - 负载均衡

3. **监控和日志**
   - 任务执行状态监控
   - 执行历史记录
   - 详细的执行日志

## 使用Docker部署

### 前置条件

- Docker 19.03+
- Docker Compose 1.27+

### 快速开始

1. **克隆代码仓库**

```bash
git clone <repository-url>
cd <repository-directory>
```

2. **启动系统**

```bash
docker-compose up -d
```

3. **访问系统**

- Web UI: http://localhost
- API接口: http://localhost:8080/api/stats

### 详细部署说明

请参考 [Docker部署指南](docker/README.md) 获取更详细的部署和配置说明。

## 系统截图

### 仪表盘
![仪表盘](docs/images/dashboard.png)

### 任务管理
![任务管理](docs/images/job-list.png)

### 执行器管理
![执行器管理](docs/images/executor-list.png)

## 技术栈

- **后端**
  - C++17
  - MySQL
  - Kafka
  - ZooKeeper
  - cpp-httplib

- **前端**
  - Vue 3
  - TypeScript
  - Element Plus
  - ECharts

## 许可证

MIT

## 贡献

欢迎提交问题和拉取请求。