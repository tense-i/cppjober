# FindRdKafka.cmake
# 查找RdKafka库
#
# 定义以下变量:
# RdKafka_FOUND - 系统中是否找到RdKafka
# RdKafka_INCLUDE_DIRS - RdKafka头文件目录
# RdKafka_LIBRARIES - 链接RdKafka所需的库

find_path(RdKafka_INCLUDE_DIR
  NAMES librdkafka/rdkafka.h
  PATHS /usr/include /usr/local/include
)

find_library(RdKafka_LIBRARY
  NAMES rdkafka
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

find_library(RdKafka_CPP_LIBRARY
  NAMES rdkafka++
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RdKafka DEFAULT_MSG
  RdKafka_INCLUDE_DIR
  RdKafka_LIBRARY
  RdKafka_CPP_LIBRARY
)

if(RdKafka_FOUND)
  set(RdKafka_INCLUDE_DIRS ${RdKafka_INCLUDE_DIR})
  set(RdKafka_LIBRARIES ${RdKafka_CPP_LIBRARY} ${RdKafka_LIBRARY})
  
  if(NOT TARGET RdKafka::rdkafka)
    add_library(RdKafka::rdkafka UNKNOWN IMPORTED)
    set_target_properties(RdKafka::rdkafka PROPERTIES
      IMPORTED_LOCATION "${RdKafka_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${RdKafka_INCLUDE_DIRS}"
    )
  endif()
  
  if(NOT TARGET RdKafka::rdkafka++)
    add_library(RdKafka::rdkafka++ UNKNOWN IMPORTED)
    set_target_properties(RdKafka::rdkafka++ PROPERTIES
      IMPORTED_LOCATION "${RdKafka_CPP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${RdKafka_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES RdKafka::rdkafka
    )
  endif()
endif()

mark_as_advanced(
  RdKafka_INCLUDE_DIR
  RdKafka_LIBRARY
  RdKafka_CPP_LIBRARY
) 