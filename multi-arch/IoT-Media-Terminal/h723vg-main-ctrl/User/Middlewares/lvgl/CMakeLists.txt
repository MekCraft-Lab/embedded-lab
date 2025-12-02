cmake_minimum_required(VERSION 3.12.4)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(NOT ESP_PLATFORM)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    project(
      lvgl
      LANGUAGES C
      HOMEPAGE_URL https://github.com/lvgl/lvgl)
  endif()
  if(NOT (CMAKE_C_COMPILER_ID STREQUAL "MSVC"))
    enable_language(CXX ASM)
  else()
    enable_language(CXX)
  endif()
endif()

set(LVGL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

#[[
    unfortunately CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS does not work for global data.
    for global data we still need decl specs.
    Check out the docs to learn more about the limitations of CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS
    https://cmake.org/cmake/help/latest/prop_tgt/WINDOWS_EXPORT_ALL_SYMBOLS.html#prop_tgt:WINDOWS_EXPORT_ALL_SYMBOLS

    For all compiled sources within the library (i.e. basically all lvgl files) we need to use dllexport.
    For all compiled sources from outside the library (i.e. files which include lvgl headers) we need to use dllimport.
    We can do this by using CMakes INTERFACE and PRIVATE keyword.
  ]]
if(MSVC AND BUILD_SHARED_LIBS)
  target_compile_definitions(
    lvgl
    INTERFACE "LV_ATTRIBUTE_EXTERN_DATA=__declspec(dllimport)"
    PRIVATE "LV_ATTRIBUTE_EXTERN_DATA=__declspec(dllexport)")
endif()

file(GLOB_RECURSE LVGL_SRC
        "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/core/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/draw/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/extra/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/font/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/hal/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/misc/*.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/widgets/*.c"
)

set(MX_Include_Dirs
        ${CMAKE_CURRENT_LIST_DIR}/../../../Core/Inc
        ${CMAKE_CURRENT_LIST_DIR}/../../../Drivers/STM32H7xx_HAL_Driver/Inc
        ${CMAKE_CURRENT_LIST_DIR}/../../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
        ${CMAKE_CURRENT_LIST_DIR}/../../../Middlewares/Third_Party/FreeRTOS/Source/include
        ${CMAKE_CURRENT_LIST_DIR}/../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
        ${CMAKE_CURRENT_LIST_DIR}/../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
        ${CMAKE_CURRENT_LIST_DIR}/../../../Drivers/CMSIS/Device/ST/STM32H7xx/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../../Drivers/CMSIS/Include
)

add_library(lvgl STATIC ${LVGL_SRC})


target_compile_definitions(lvgl PUBLIC
        LV_CONF_INCLUDE_SIMPLE
)

target_include_directories(lvgl PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${MX_Include_Dirs}
)