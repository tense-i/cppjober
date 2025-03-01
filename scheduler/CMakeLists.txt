set(SCHEDULER_SOURCES
    src/scheduler.cpp
)

set(SCHEDULER_HEADERS
    include/scheduler.h
)

add_library(scheduler STATIC ${SCHEDULER_SOURCES} ${SCHEDULER_HEADERS})

target_include_directories(scheduler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 查找pthread库
find_package(Threads REQUIRED)

target_link_libraries(scheduler
    PUBLIC
        common
        Threads::Threads
)

# 添加调度器可执行文件
add_executable(scheduler_app main.cpp)
target_link_libraries(scheduler_app PRIVATE scheduler)

# 启用测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif() 