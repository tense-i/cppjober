-- 更新执行器节点表，添加负载信息字段
ALTER TABLE executor_node 
ADD COLUMN current_load INT NOT NULL DEFAULT 0 COMMENT '当前负载（正在执行的任务数）',
ADD COLUMN max_load INT NOT NULL DEFAULT 10 COMMENT '最大负载容量',
ADD COLUMN total_tasks_executed BIGINT NOT NULL DEFAULT 0 COMMENT '已执行任务总数',
ADD INDEX idx_current_load (current_load);

-- 更新现有记录，设置默认最大负载
UPDATE executor_node SET max_load = 10 WHERE max_load = 0; 