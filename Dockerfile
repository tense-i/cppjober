# 使用多阶段构建
FROM ubuntu:20.04 as builder

# 设置时区
ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 安装基础工具
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# 安装vcpkg
WORKDIR /opt
RUN git clone https://github.com/Microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

# 设置vcpkg环境变量
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# 复制vcpkg配置文件
COPY vcpkg.json /app/
WORKDIR /app

# 安装依赖
RUN vcpkg install --triplet x64-linux

# 复制源代码
COPY . /app/

# 构建项目
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake && \
    make -j$(nproc)

# 运行阶段
FROM ubuntu:20.04

# 设置时区
ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libgcc1 \
    && rm -rf /var/lib/apt/lists/*

# 复制构建产物
COPY --from=builder /app/build/libscheduler.a /usr/local/lib/
COPY --from=builder /app/include /usr/local/include/scheduler
COPY --from=builder /app/config /etc/scheduler

# 设置工作目录
WORKDIR /app

# 设置环境变量
ENV LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH}

# 暴露端口
EXPOSE 8080

# 启动命令
CMD ["./scheduler_service"] 