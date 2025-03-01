# 分布式任务调度系统 Docker 部署指南

本文档提供了使用 Docker 和 Docker Compose 部署分布式任务调度系统的详细说明。

## 系统架构

系统由以下组件组成：

- **MySQL**: 数据存储
- **ZooKeeper**: 分布式协调服务
- **Kafka**: 消息队列
- **调度器(Scheduler)**: 负责任务调度和分发
- **执行器(Executor)**: 负责执行任务
- **Web UI**: 提供用户界面

## 前置条件

- Docker 19.03+
- Docker Compose 1.27+
- Git

## 目录结构

```
.
├── Dockerfile                # 主Dockerfile（调度器和执行器）
├── docker-compose.yml        # Docker Compose配置文件
├── docker/
│   ├── config/               # 配置文件目录
│   │   ├── scheduler.conf    # 调度器配置
│   │   └── executor.conf     # 执行器配置
│   ├── mysql/                # MySQL相关文件
│   │   └── init.sql          # 数据库初始化脚本
│   └── scripts/              # 启动脚本
│       ├── start-scheduler.sh # 调度器启动脚本
│       └── start-executor.sh  # 执行器启动脚本
└── web-ui/                   # 前端UI目录
    ├── Dockerfile            # 前端UI的Dockerfile
    └── nginx.conf            # Nginx配置文件
```

## 构建和启动

1. 克隆代码仓库：

```bash
git clone <repository-url>
cd <repository-directory>
```

2. 创建必要的目录：

```bash
mkdir -p docker/config docker/mysql docker/scripts
```

3. 确保脚本有执行权限：

```bash
chmod +x docker/scripts/start-scheduler.sh
chmod +x docker/scripts/start-executor.sh
```

4. 使用Docker Compose构建和启动系统：

```bash
docker-compose up -d
```

5. 查看服务状态：

```bash
docker-compose ps
```

## 访问系统

- **Web UI**: http://localhost
- **API接口**: http://localhost:8080/api/stats

## 查看日志

```bash
# 查看调度器日志
docker-compose logs -f scheduler

# 查看执行器日志
docker-compose logs -f executor

# 查看Web UI日志
docker-compose logs -f web-ui
```

## 停止系统

```bash
docker-compose down
```

如果需要同时删除数据卷（会删除所有数据）：

```bash
docker-compose down -v
```

## 配置说明

### 调度器配置 (scheduler.conf)

主要配置项：

- `db.host`, `db.port`, `db.user`, `db.password`, `db.name`: 数据库连接信息
- `kafka.brokers`: Kafka服务器地址
- `scheduler.executor_selection_strategy`: 执行器选择策略（RANDOM, ROUND_ROBIN, LEAST_LOAD）
- `scheduler.check_interval`: 调度检查间隔（秒）
- `stats.api.port`: 统计API端口

### 执行器配置 (executor.conf)

主要配置项：

- `db.host`, `db.port`, `db.user`, `db.password`, `db.name`: 数据库连接信息
- `kafka.brokers`: Kafka服务器地址
- `executor.default_max_load`: 执行器默认最大负载
- `executor.heartbeat_interval`: 心跳间隔（秒）

## 故障排除

1. 如果服务无法启动，请检查日志：

```bash
docker-compose logs <service-name>
```

2. 确保所有必要的端口未被占用：
   - 3306 (MySQL)
   - 2181 (ZooKeeper)
   - 9092 (Kafka)
   - 8080 (调度器API)
   - 80 (Web UI)

3. 如果需要重新构建服务：

```bash
docker-compose build --no-cache <service-name>
```

## 扩展执行器

要添加更多执行器，可以修改`docker-compose.yml`文件，添加更多执行器服务：

```yaml
executor-2:
  build:
    context: .
    dockerfile: Dockerfile
  container_name: scheduler-executor-2
  restart: always
  depends_on:
    - scheduler
  volumes:
    - ./docker/config:/config
    - ./docker/scripts:/scripts
  command: ["/scripts/start-executor.sh"]
  networks:
    - scheduler-network
``` 