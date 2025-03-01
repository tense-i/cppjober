FROM ubuntu:20.04

# 设置时区
ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 安装依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libmysqlclient-dev \
    librdkafka-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    uuid-dev \
    pkg-config \
    nlohmann-json3-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

    # 安装etcd-cpp-api
RUN apt-get update && apt-get install -y \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

# 克隆并构建etcd-cpp-api
WORKDIR /tmp
RUN git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git \
    && cd etcd-cpp-apiv3 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j4 \
    && make install \
    && ldconfig
    
# 设置工作目录
WORKDIR /app

# 复制源代码
COPY . /app/

# 创建构建目录
RUN mkdir -p /app/build

# 构建项目
WORKDIR /app/build
RUN cmake .. && make -j4

# 设置环境变量
ENV LD_LIBRARY_PATH=/usr/local/lib

# 暴露端口
EXPOSE 8080

# 设置入口点
ENTRYPOINT ["/bin/bash"] 