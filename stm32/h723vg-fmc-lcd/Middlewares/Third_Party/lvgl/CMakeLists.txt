cmake_minimum_required(VERSION 3.5)

project(lvgl LANGUAGES C)

set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h")


# 收集 LVGL 源文件
file(GLOB_RECURSE LVGL_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/core/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/draw/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/extra/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/font/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/hal/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/misc/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/widgets/*.c"

)


add_library(lvgl STATIC ${LVGL_SOURCES})

# 添加 include 路径
target_include_directories(lvgl PUBLIC

        /src
        "${CMAKE_CURRENT_LIST_DIR}/"

)

message(LV_CONF_PATH的值为)
message(${LV_CONF_PATH})


# STM32 不要 desktop 环境，不要启用 os_desktop.cmake
add_definitions(-DLV_CONF_PATH="${LV_CONF_PATH}")
