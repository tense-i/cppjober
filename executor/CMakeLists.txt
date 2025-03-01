set(EXECUTOR_SOURCES
    src/executor.cpp
)

set(EXECUTOR_HEADERS
    include/executor.h
)

add_library(executor STATIC ${EXECUTOR_SOURCES} ${EXECUTOR_HEADERS})

target_include_directories(executor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 查找pthread库
find_package(Threads REQUIRED)

target_link_libraries(executor
    PUBLIC
        common
        Threads::Threads
)

# 添加执行器可执行文件
add_executable(executor_app main.cpp)
target_link_libraries(executor_app PRIVATE 
    executor
    uuid
)

# 启用测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif() 