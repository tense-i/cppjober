# Findspdlog.cmake
# 查找spdlog库
#
# 定义以下变量:
# spdlog_FOUND - 系统中是否找到spdlog
# spdlog_INCLUDE_DIRS - spdlog头文件目录
# spdlog_LIBRARIES - 链接spdlog所需的库

find_path(spdlog_INCLUDE_DIR
  NAMES spdlog/spdlog.h
  PATHS /usr/include /usr/local/include
)

find_library(spdlog_LIBRARY
  NAMES spdlog
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog DEFAULT_MSG
  spdlog_INCLUDE_DIR
  spdlog_LIBRARY
)

if(spdlog_FOUND)
  set(spdlog_INCLUDE_DIRS ${spdlog_INCLUDE_DIR})
  set(spdlog_LIBRARIES ${spdlog_LIBRARY})
  
  if(NOT TARGET spdlog::spdlog)
    add_library(spdlog::spdlog UNKNOWN IMPORTED)
    set_target_properties(spdlog::spdlog PROPERTIES
      IMPORTED_LOCATION "${spdlog_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${spdlog_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(
  spdlog_INCLUDE_DIR
  spdlog_LIBRARY
) 