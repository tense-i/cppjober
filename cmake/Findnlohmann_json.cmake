# Findnlohmann_json.cmake
# 查找nlohmann_json库
#
# 定义以下变量:
# nlohmann_json_FOUND - 系统中是否找到nlohmann_json
# nlohmann_json_INCLUDE_DIRS - nlohmann_json头文件目录

find_path(nlohmann_json_INCLUDE_DIR
  NAMES nlohmann/json.hpp
  PATHS /usr/include /usr/local/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nlohmann_json DEFAULT_MSG
  nlohmann_json_INCLUDE_DIR
)

if(nlohmann_json_FOUND)
  set(nlohmann_json_INCLUDE_DIRS ${nlohmann_json_INCLUDE_DIR})
  
  if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(nlohmann_json_INCLUDE_DIR) 