#!/bin/bash

# 等待MySQL和Kafka启动
echo "等待MySQL和Kafka启动..."
sleep 10

# 创建配置目录
mkdir -p /app/config

# 复制配置文件
cp /config/scheduler.conf /app/config/

# 启动调度器
cd /app/build/scheduler
echo "启动调度器..."
./scheduler_app /app/config/scheduler.conf 