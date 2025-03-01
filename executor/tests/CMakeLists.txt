# 查找GTest包
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 添加测试可执行文件
add_executable(job_cancel_test 
    job_cancel_test.cpp
    mock_executor.h
)

# 链接测试库
target_link_libraries(job_cancel_test
    PRIVATE
        executor
        ${GTEST_BOTH_LIBRARIES}
        gmock
        pthread
)

# 包含头文件目录
target_include_directories(job_cancel_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/common/include
)

# 添加测试
add_test(NAME JobCancelTest COMMAND job_cancel_test) 