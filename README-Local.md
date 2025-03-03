# 分布式任务调度系统 - 本地部署指南

## 系统要求

### 硬件要求
- CPU: 4核及以上
- 内存: 8GB及以上
- 磁盘空间: 50GB及以上

### 软件要求
- 操作系统: Ubuntu 20.04 LTS / CentOS 7.x 及以上
- GCC/G++: 9.0及以上（支持C++17）
- CMake: 3.15及以上
- MySQL: 8.0及以上
- Kafka: 2.8.0及以上
- ZooKeeper: 3.7.0及以上
- Git

## 安装步骤

### 1. 安装基础开发工具

```bash
# Ubuntu系统
sudo apt update
sudo apt install -y build-essential cmake git

# CentOS系统
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake git
```

### 2. 安装依赖库

```bash
# Ubuntu系统
sudo apt install -y \
    libboost-all-dev \
    libmysqlclient-dev \
    libssl-dev \
    librdkafka-dev \
    libzookeeper-mt-dev

# CentOS系统
sudo yum install -y \
    boost-devel \
    mysql-devel \
    openssl-devel \
    librdkafka-devel \
    zookeeper-native
```

### 3. 安装并配置MySQL

```bash
# Ubuntu系统
sudo apt install -y mysql-server mysql-client

# CentOS系统
sudo yum install -y mysql-server mysql-client

# 启动MySQL服务
sudo systemctl start mysql
sudo systemctl enable mysql

# 配置MySQL
sudo mysql_secure_installation
```

### 4. 安装并配置Kafka

```bash
# 下载Kafka
wget https://downloads.apache.org/kafka/2.8.0/kafka_2.13-2.8.0.tgz
tar -xzf kafka_2.13-2.8.0.tgz
sudo mv kafka_2.13-2.8.0 /opt/kafka

# 配置Kafka服务
sudo tee /etc/systemd/system/zookeeper.service <<EOF
[Unit]
Description=Apache ZooKeeper server
Documentation=http://zookeeper.apache.org
Requires=network.target remote-fs.target
After=network.target remote-fs.target

[Service]
Type=simple
ExecStart=/opt/kafka/bin/zookeeper-server-start.sh /opt/kafka/config/zookeeper.properties
ExecStop=/opt/kafka/bin/zookeeper-server-stop.sh
Restart=on-abnormal

[Install]
WantedBy=multi-user.target
EOF

sudo tee /etc/systemd/system/kafka.service <<EOF
[Unit]
Description=Apache Kafka Server
Documentation=http://kafka.apache.org/documentation.html
Requires=zookeeper.service
After=zookeeper.service

[Service]
Type=simple
ExecStart=/opt/kafka/bin/kafka-server-start.sh /opt/kafka/config/server.properties
ExecStop=/opt/kafka/bin/kafka-server-stop.sh
Restart=on-abnormal

[Install]
WantedBy=multi-user.target
EOF

# 启动服务
sudo systemctl start zookeeper
sudo systemctl enable zookeeper
sudo systemctl start kafka
sudo systemctl enable kafka
```

### 5. 编译和安装系统

```bash
# 克隆代码仓库
git clone <repository-url>
cd <repository-directory>

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)

# 安装
sudo make install
```

## 配置说明

### 1. 数据库配置

```bash
# 创建数据库和用户
mysql -u root -p

CREATE DATABASE job_scheduler;
CREATE USER 'scheduler'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON job_scheduler.* TO 'scheduler'@'localhost';
FLUSH PRIVILEGES;

# 导入数据库架构
mysql -u scheduler -p job_scheduler < ../sql/schema.sql
```

### 2. 系统配置

编辑配置文件 `/etc/job-scheduler/config.json`：

```json
{
    "scheduler": {
        "port": 8080,
        "threads": 4,
        "log_level": "info"
    },
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "scheduler",
        "password": "your_password",
        "database": "job_scheduler"
    },
    "kafka": {
        "brokers": "localhost:9092",
        "group_id": "scheduler_group"
    },
    "zookeeper": {
        "hosts": "localhost:2181",
        "timeout": 5000
    }
}
```

## 启动服务

### 1. 启动调度器

```bash
sudo systemctl start scheduler
sudo systemctl enable scheduler
```

### 2. 启动执行器

```bash
sudo systemctl start executor
sudo systemctl enable executor
```

### 3. 启动Web UI

```bash
cd web-ui
npm install
npm run serve
```

## 验证部署

1. 访问Web UI: http://localhost:8080
2. 检查服务状态：
```bash
sudo systemctl status scheduler
sudo systemctl status executor
```
3. 查看日志：
```bash
sudo journalctl -u scheduler -f
sudo journalctl -u executor -f
```

## 常见问题

### 1. 服务无法启动
- 检查配置文件权限
- 确认端口未被占用
- 查看系统日志

### 2. 数据库连接失败
- 验证数据库服务状态
- 检查用户名和密码
- 确认数据库权限

### 3. Kafka连接问题
- 确认Kafka和ZooKeeper服务状态
- 检查网络连接
- 验证Topic权限

## 性能调优

### 1. MySQL优化
```ini
# /etc/mysql/my.cnf
innodb_buffer_pool_size = 4G
innodb_log_file_size = 256M
innodb_flush_log_at_trx_commit = 2
```

### 2. Kafka优化
```properties
# /opt/kafka/config/server.properties
num.network.threads=8
num.io.threads=16
socket.send.buffer.bytes=102400
socket.receive.buffer.bytes=102400
```

### 3. 系统优化
```bash
# /etc/sysctl.conf
fs.file-max = 100000
net.core.somaxconn = 65535
```

## 监控和维护

1. 设置日志轮转
2. 配置系统监控
3. 定期备份数据
4. 性能指标收集

## 安全建议

1. 更改默认密码
2. 配置防火墙规则
3. 启用SSL/TLS
4. 实施访问控制

## 技术支持

如遇到问题，请：
1. 查看详细日志
2. 检查系统状态
3. 参考故障排除指南
4. 提交Issue获取支持 