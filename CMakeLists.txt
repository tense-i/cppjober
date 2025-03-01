cmake_minimum_required(VERSION 3.10)

# 设置vcpkg工具链
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(distributed_scheduler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加cmake模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# 添加外部依赖
# 使用系统安装的MySQL库
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

find_package(RdKafka REQUIRED)
find_package(etcd-cpp-api REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# 添加GTest支持
find_package(GTest)
if(NOT GTEST_FOUND)
    message(STATUS "GTest not found, tests will be disabled")
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
else()
    message(STATUS "GTest found, tests will be enabled")
    set(BUILD_TESTS ON CACHE BOOL "Build tests" FORCE)
endif()

# 添加子目录
add_subdirectory(common)
add_subdirectory(scheduler)
add_subdirectory(executor) 