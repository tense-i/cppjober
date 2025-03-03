# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(scheduler VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
add_compile_options(-Wall -Wextra -Wpedantic)

# 设置vcpkg工具链
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# 查找依赖包
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(RdKafka CONFIG REQUIRED)
find_package(unofficial-libmysql CONFIG REQUIRED)
find_package(unofficial-zookeeper CONFIG REQUIRED)

# 添加源文件
set(SOURCES
    src/scheduler.cpp
    src/job_dao.cpp
    src/kafka_message_queue.cpp
    src/executor_api.cpp
    src/job_api.cpp
    src/stats_api.cpp
    src/config_manager.cpp
    src/stats_manager.cpp
    src/cron_parser.cpp
    src/zk_client.cpp
    src/zk_registry.cpp
)

# 添加头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# 创建库目标
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# 链接依赖库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    RdKafka::rdkafka++
    unofficial::libmysql::libmysql
    unofficial::zookeeper::zookeeper
    pthread
)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION include/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h"
)

# 添加测试
enable_testing()
add_subdirectory(tests) 