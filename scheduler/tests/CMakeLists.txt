# 查找GTest包
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 添加测试可执行文件
add_executable(executor_selection_test 
    executor_selection_test.cpp
)

# 链接测试库
target_link_libraries(executor_selection_test
    PRIVATE
        scheduler
        ${GTEST_BOTH_LIBRARIES}
        gmock
        pthread
)

# 包含头文件目录
target_include_directories(executor_selection_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/common/include
)

# 添加测试
add_test(NAME ExecutorSelectionTest COMMAND executor_selection_test) 