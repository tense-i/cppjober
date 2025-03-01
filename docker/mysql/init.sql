-- 数据库创建
CREATE DATABASE IF NOT EXISTS distributed_scheduler;
USE distributed_scheduler;

-- 任务信息表
CREATE TABLE IF NOT EXISTS job_info (
    job_id VARCHAR(64) PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    command TEXT NOT NULL,
    job_type ENUM('ONCE', 'PERIODIC') NOT NULL,
    priority INT NOT NULL DEFAULT 0,
    cron_expression VARCHAR(100),
    timeout INT NOT NULL DEFAULT 60,
    retry_count INT NOT NULL DEFAULT 0,
    retry_interval INT NOT NULL DEFAULT 0,
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_priority (priority),
    INDEX idx_job_type (job_type)
);

-- 任务执行记录表
CREATE TABLE IF NOT EXISTS job_execution (
    execution_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    job_id VARCHAR(64) NOT NULL,
    executor_id VARCHAR(64),
    status ENUM('WAITING', 'RUNNING', 'SUCCESS', 'FAILED', 'TIMEOUT') NOT NULL,
    start_time TIMESTAMP NULL,
    end_time TIMESTAMP NULL,
    output TEXT,
    error TEXT,
    retry_count INT NOT NULL DEFAULT 0,
    trigger_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_job_id (job_id),
    INDEX idx_status (status),
    INDEX idx_trigger_time (trigger_time),
    FOREIGN KEY (job_id) REFERENCES job_info(job_id) ON DELETE CASCADE
);

-- 执行器节点表
CREATE TABLE IF NOT EXISTS executor_node (
    executor_id VARCHAR(64) PRIMARY KEY,
    host VARCHAR(255) NOT NULL,
    port INT NOT NULL,
    status ENUM('ONLINE', 'OFFLINE') NOT NULL,
    last_heartbeat TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    register_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    current_load INT NOT NULL DEFAULT 0 COMMENT '当前负载（正在执行的任务数）',
    max_load INT NOT NULL DEFAULT 10 COMMENT '最大负载容量',
    total_tasks_executed BIGINT NOT NULL DEFAULT 0 COMMENT '已执行任务总数',
    INDEX idx_status (status),
    INDEX idx_last_heartbeat (last_heartbeat),
    INDEX idx_current_load (current_load)
);

-- 任务锁表（用于分布式锁）
CREATE TABLE IF NOT EXISTS job_lock (
    lock_name VARCHAR(64) PRIMARY KEY,
    lock_owner VARCHAR(64) NOT NULL,
    lock_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expire_time TIMESTAMP NOT NULL,
    INDEX idx_expire_time (expire_time)
);

-- 系统配置表
CREATE TABLE IF NOT EXISTS system_config (
    config_key VARCHAR(64) PRIMARY KEY,
    config_value TEXT NOT NULL,
    description VARCHAR(255),
    update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- 初始化系统配置
INSERT INTO system_config (config_key, config_value, description) VALUES
('job_cleanup_days', '30', '任务执行记录保留天数'),
('default_timeout', '60', '默认任务超时时间（秒）'),
('max_retry_count', '3', '最大重试次数'),
('retry_interval', '30', '默认重试间隔（秒）'); 