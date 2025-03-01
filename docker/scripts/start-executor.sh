#!/bin/bash

# 等待MySQL、Kafka和调度器启动
echo "等待MySQL、Kafka和调度器启动..."
sleep 15

# 创建配置目录
mkdir -p /app/config

# 复制配置文件
cp /config/executor.conf /app/config/

# 启动执行器
cd /app/build/executor
echo "启动执行器..."
./executor_app /app/config/executor.conf 