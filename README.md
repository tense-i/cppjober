# 分布式任务调度系统

基于C++17实现的分布式任务调度系统。

## 系统架构

系统主要包含以下组件：

1. 调度器(Scheduler)
   - 负责任务的调度和分发
   - 维护执行器注册信息
   - 处理任务执行结果

2. 执行器(Executor)
   - 接收并执行任务
   - 向调度中心注册
   - 维护心跳
   - 返回执行结果

3. 公共组件(Common)
   - 任务相关数据结构
   - 数据库访问层
   - Kafka客户端
   - Etcd客户端

## 依赖

- C++17或更高版本
- CMake 3.10或更高版本
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

## 目录结构

```
.
├── CMakeLists.txt
├── README.md
├── vcpkg.json
├── common/
│   ├── include/
│   │   ├── job.h
│   │   ├── job_storage.h
│   │   ├── kafka_client.h
│   │   └── etcd_client.h
│   └── src/
├── scheduler/
│   ├── include/
│   │   └── scheduler.h
│   └── src/
└── executor/
    ├── include/
    │   └── executor.h
    ├── src/
    └── tests/
        ├── job_cancel_test.cpp
        └── mock_executor.h
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

1. 模拟类 (Mocks)
   - `MockExecutor`: 模拟执行器，用于测试任务取消功能
   - 避免在测试中实际连接数据库和Kafka

2. 测试用例
   - 任务取消测试: 验证任务取消功能在各种场景下的正确性
     - 取消不存在的任务
     - 取消队列中的任务
     - 取消多个任务
     - 取消已取消的任务

3. 测试设计原则
   - 隔离外部依赖
   - 专注于单一功能点
   - 覆盖边界情况和错误处理

### 添加新测试

要添加新的测试，请遵循以下步骤：

1. 在相应模块的`tests`目录下创建测试文件
2. 如有必要，创建模拟类以隔离外部依赖
3. 在`CMakeLists.txt`中注册新的测试可执行文件
4. 实现测试用例，确保覆盖主要功能点和边界情况

## 功能特性

1. 任务管理
   - 支持一次性任务和周期性任务
   - 基于优先级的任务调度
   - 任务超时控制
   - 失败重试机制

2. 执行器管理
   - 自动注册和发现
   - 心跳检测
   - 负载均衡

3. 监控和日志
   - 任务执行状态监控
   - 执行历史记录
   - 详细的执行日志

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