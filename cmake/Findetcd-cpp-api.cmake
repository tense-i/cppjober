# Findetcd-cpp-api.cmake
# 查找etcd-cpp-api库
#
# 定义以下变量:
# etcd-cpp-api_FOUND - 系统中是否找到etcd-cpp-api
# etcd-cpp-api_INCLUDE_DIRS - etcd-cpp-api头文件目录
# etcd-cpp-api_LIBRARIES - 链接etcd-cpp-api所需的库

find_path(etcd-cpp-api_INCLUDE_DIR
  NAMES etcd/Client.hpp
  PATHS /usr/include /usr/local/include
)

find_library(etcd-cpp-api_LIBRARY
  NAMES etcd-cpp-api
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(etcd-cpp-api DEFAULT_MSG
  etcd-cpp-api_INCLUDE_DIR
  etcd-cpp-api_LIBRARY
)

if(etcd-cpp-api_FOUND)
  set(etcd-cpp-api_INCLUDE_DIRS ${etcd-cpp-api_INCLUDE_DIR})
  set(etcd-cpp-api_LIBRARIES ${etcd-cpp-api_LIBRARY})
  
  if(NOT TARGET etcd-cpp-api)
    add_library(etcd-cpp-api UNKNOWN IMPORTED)
    set_target_properties(etcd-cpp-api PROPERTIES
      IMPORTED_LOCATION "${etcd-cpp-api_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${etcd-cpp-api_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(
  etcd-cpp-api_INCLUDE_DIR
  etcd-cpp-api_LIBRARY
) 