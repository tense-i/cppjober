set(COMMON_SOURCES
    src/job.cpp
    src/db_connection_pool.cpp
    src/job_dao.cpp
    src/kafka_message_queue.cpp
    src/cron_parser.cpp
    src/config_manager.cpp
)

set(COMMON_HEADERS
    include/job.h
    include/job_storage.h
    include/kafka_client.h
    include/etcd_client.h
    include/db_connection_pool.h
    include/job_dao.h
    include/kafka_message_queue.h
    include/cron_parser.h
    include/config_manager.h
)

add_library(common STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        ${MYSQL_LIBRARIES}
        RdKafka::rdkafka++
        etcd-cpp-api
)

target_include_directories(common
    PUBLIC
        ${MYSQL_INCLUDE_DIRS}
)

# 添加Cron测试可执行文件
add_executable(cron_test src/cron_test.cpp)
target_link_libraries(cron_test PRIVATE common)

# 启用测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif() 