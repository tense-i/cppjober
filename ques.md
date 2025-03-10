分布式任务调度系统
题目描述
设计并实现一个分布式任务调度系统，支持任务的创建、调度、执行和监控。系统需要支持
以下功能：
1. 任务创建：
o 用户可以创建任务，任务包括任务名称、执行命令、执行时间和执行频率。
o 任务可以是一次性的，也可以是周期性的。
2. 任务调度：
o 系统需要根据任务的执行时间和频率，自动调度任务到可用的工作节点。
o 支持任务的优先级调度。
3. 任务执行：
o 工作节点接收任务并执行，执行结果需要返回给调度系统。
o 支持任务的超时重试和失败重试。
4. 任务监控：
o 用户可以查看任务的执行状态、执行结果和执行日志。
o 系统需要提供任务执行的历史记录和统计信息。
技术要求
1. 后端：
o 使用C++或Java实现调度系统和工作节点。
o 使用Zookeeper或Etcd进行分布式协调。
o 使用消息队列（如Kafka或RabbitMQ）进行任务的分发和结果返回。
2. 前端：
o 使用React或Vue实现前端页面。
o 支持任务的创建、调度、执行和监控。
3. 部署：
o 使用Docker容器化部署调度系统和工作节点。
o 使用Prometheus和Grafana进行系统监控。
评分标准
1. 功能完整性（40分）：
o 任务创建、调度、执行、监控功能是否完整实现。
2. 性能优化（30分）：
o 系统是否支持高并发任务调度，是否使用了消息队列和分布式协调优化性能。
3. 代码质量（20分）：
o 代码是否清晰、易读，是否有良好的注释和文档。
4. 部署与测试（10分）：
o 系统是否成功部署，是否有自动化测试用例。
