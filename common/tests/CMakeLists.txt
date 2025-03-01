# 查找GTest包
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 添加测试可执行文件
add_executable(cron_parser_test 
    cron_parser_test.cpp
)

# 链接测试库
target_link_libraries(cron_parser_test
    PRIVATE
        common
        ${GTEST_BOTH_LIBRARIES}
        gmock
        pthread
)

# 包含头文件目录
target_include_directories(cron_parser_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 添加测试
add_test(NAME CronParserTest COMMAND cron_parser_test) 