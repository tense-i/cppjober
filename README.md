# 分布式任务调度系统

基于C++17实现的高性能、可扩展的分布式任务调度系统。

## 系统概述

本系统旨在提供一个高可用、可扩展的分布式任务调度平台，支持大规模任务的调度、执行和监控。系统具备以下特点：

- **高可靠性**：保证任务的可靠执行，支持失败重试
- **高可用性**：无单点故障，支持水平扩展
- **灵活性**：支持多种任务类型和调度策略
- **可观测性**：提供完善的监控和日志功能

### 应用场景

- 批处理作业调度
- 定时数据处理
- 系统维护任务
- 分布式计算任务协调
- ETL数据处理流程

## 系统架构

系统采用Master-Worker架构，主要包含以下组件：

1. **调度器(Scheduler)**
   - 负责任务的调度和分发
   - 维护执行器注册信息
   - 处理任务执行结果
   - 提供API接口

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
   - **MySQL**: 存储任务定义和执行记录
   - **Kafka**: 用于任务分发和结果回传
   - **ZooKeeper**: 用于服务发现和协调

### 架构图

![系统架构图](doc/design_document.assets/image-20250303134507219.png)

## 功能特性

1. **任务管理**
   - 支持一次性任务和周期性任务
   - 基于优先级的任务调度
   - 任务超时控制
   - 失败重试机制
   - 任务状态跟踪

2. **执行器管理**
   - 自动注册和发现
   - 心跳检测
   - 负载均衡
   - 动态扩缩容

3. **监控和日志**
   - 任务执行状态监控
   - 执行历史记录
   - 详细的执行日志
   - 性能指标收集

4. **安全特性**
   - 基于角色的访问控制
   - 数据传输加密
   - 敏感数据保护

## 技术栈

- **后端**
  - C++17
  - MySQL 8.0+
  - Kafka 2.8.0+
  - ZooKeeper 3.7.0+
  - CMake 3.15+

- **前端**
  - Vue 3
  - TypeScript
  - Element Plus
  - ECharts

## 依赖

- C++17或更高版本
- CMake 3.15或更高版本
- vcpkg包管理器

### 安装vcpkg

```bash
# Windows
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Linux/macOS
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install

# 设置VCPKG_ROOT环境变量
# Windows (PowerShell)
$env:VCPKG_ROOT="path/to/vcpkg"
# Linux/macOS
export VCPKG_ROOT="path/to/vcpkg"
```

### 安装依赖

所有依赖都通过vcpkg自动安装，包括：
- libmysql 8.0.31+
- librdkafka 2.1.0+
- etcd-cpp-apiv3 0.8.0+
- nlohmann-json 3.11.2+
- spdlog 1.12.0+

## 编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# 编译
cmake --build build --config Release
```

## 部署方式

系统支持两种部署方式：

### 1. Docker部署

使用Docker和Docker Compose进行容器化部署，详见[Docker部署指南](README-Docker.md)。

### 2. 本地部署

在本地环境直接部署，详见[本地部署指南](README-Local.md)。

> **推荐使用本地部署**：Docker compose中的容器依赖了很多第三方库，由于网络原因，拉取可能较慢。

## 目录结构

```
.
├── CMakeLists.txt          # 主CMake配置文件
├── README.md               # 项目说明文档
├── README-Docker.md        # Docker部署指南
├── README-Local.md         # 本地部署指南
├── vcpkg.json              # vcpkg依赖配置
├── docker-compose.yml      # Docker Compose配置
├── Dockerfile              # Docker构建文件
├── common/                 # 公共组件
│   ├── include/            # 头文件
│   │   ├── job.h           # 任务相关数据结构
│   │   ├── job_storage.h   # 任务存储接口
│   │   ├── kafka_client.h  # Kafka客户端
│   │   └── etcd_client.h   # Etcd客户端
│   └── src/                # 源文件
├── scheduler/              # 调度器组件
│   ├── include/            # 头文件
│   │   └── scheduler.h     # 调度器接口
│   └── src/                # 源文件
├── executor/               # 执行器组件
│   ├── include/            # 头文件
│   │   └── executor.h      # 执行器接口
│   ├── src/                # 源文件
│   └── tests/              # 测试文件
│       ├── job_cancel_test.cpp  # 任务取消测试
│       └── mock_executor.h      # 模拟执行器
├── web-ui/                 # Web界面
├── config/                 # 配置文件
├── doc/                    # 文档
│   ├── README.md           # 文档索引
│   └── design_document.md  # 设计说明书
└── docker/                 # Docker相关文件
```

## 数据库设计

### jobs表
```sql
CREATE TABLE jobs (
    job_id VARCHAR(64) PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    command TEXT NOT NULL,
    type TINYINT NOT NULL,
    priority INT DEFAULT 0,
    cron_expression VARCHAR(100),
    timeout INT DEFAULT 0,
    retry_count INT DEFAULT 0,
    retry_interval INT DEFAULT 0,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

### job_logs表
```sql
CREATE TABLE job_logs (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    job_id VARCHAR(64) NOT NULL,
    executor_id VARCHAR(64) NOT NULL,
    status TINYINT NOT NULL,
    start_time DATETIME NOT NULL,
    end_time DATETIME,
    output TEXT,
    error TEXT,
    INDEX idx_job_id (job_id),
    INDEX idx_start_time (start_time)
);
```

### executors表
```sql
CREATE TABLE executors (
    executor_id VARCHAR(64) PRIMARY KEY,
    host VARCHAR(255) NOT NULL,
    port INT NOT NULL,
    status TINYINT NOT NULL,
    last_heartbeat_time DATETIME NOT NULL,
    register_time DATETIME NOT NULL,
    INDEX idx_status (status)
);
```

## 测试框架

本项目使用Google Test和Google Mock进行单元测试和集成测试。

### 测试依赖

- Google Test (gtest)
- Google Mock (gmock)

### 运行测试

```bash
# 在构建目录中运行所有测试
cd build
ctest

# 运行特定测试
cd build
./executor/tests/job_cancel_test
```

### 测试模块结构

1. **模拟类 (Mocks)**
   - `MockExecutor`: 模拟执行器，用于测试任务取消功能
   - 避免在测试中实际连接数据库和Kafka

2. **测试用例**
   - 任务取消测试: 验证任务取消功能在各种场景下的正确性
     - 取消不存在的任务
     - 取消队列中的任务
     - 取消多个任务
     - 取消已取消的任务

3. **测试设计原则**
   - 隔离外部依赖
   - 专注于单一功能点
   - 覆盖边界情况和错误处理

## 文档

系统提供了详细的文档，包括：

- [设计说明书](doc/design_document.md) - 系统的详细设计，包含架构设计、组件设计、接口设计等
- [Docker部署指南](README-Docker.md) - Docker容器化部署说明
- [本地部署指南](README-Local.md) - 本地环境部署说明

设计说明书中包含了系统的UML图表，包括：
- 类图
- 活动图
- 数据流图
- 用例图
- 时序图

## 性能与可扩展性

### 性能优化

- 数据库索引优化
- 连接池管理
- 任务批量处理
- 缓存机制

### 可扩展性设计

- 调度器集群：支持多个调度器实例，通过ZooKeeper协调
- 执行器水平扩展：可以动态增加执行器节点
- 数据库分库分表：支持数据库水平扩展
- 消息队列分区：Kafka主题分区，提高并行处理能力

## 贡献指南

欢迎贡献代码和提交问题！请遵循以下步骤：

1. Fork项目
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建Pull Request

## 许可证

MIT

## 联系方式

如有问题或建议，请提交Issue或联系项目维护者。 

