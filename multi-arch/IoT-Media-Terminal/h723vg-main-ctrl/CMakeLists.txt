cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME h723vg-main-ctrl)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(cmake/gcc-arm-none-eabi.cmake)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        User/Applications/user-zone-entry.cpp
        User/Applications/user-zone-entry.h
        User/Applications/Service/service_explorer.cpp
        User/Applications/app-fileSystem.cpp
        User/Applications/app-host.cpp
        User/Applications/app-mallocFailed.cpp
        User/Applications/app-stackOverflow.cpp
        User/Drivers/Devices/W25Qxx/w25qxx.cpp
        User/Drivers/Devices/W25Qxx/w25qxx-comm-adpter.cpp
        User/Middlewares/LittleFs/Adapters/adapter-lfs.cpp
        User/Middlewares/LittleFs/lfs.c
        User/Middlewares/LittleFs/lfs_util.c
        User/Middlewares/Proto/proto-mek.cpp
        User/Drivers/Devices/ILI9481/drv-ili9481.cpp
        User/Drivers/Devices/ILI9481/drv-ili9481-lvgl-adaptor.cpp
        User/Applications/app-graphic.cpp
        User/Applications/app-createTimer.cpp
        User/Middlewares/lvgl/lv_port_disp.c
        User/Applications/app-wirelessComm.cpp
        User/Applications/app-motionCtrl.cpp)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/User"
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

#添加LVGL库
add_subdirectory(User/Middlewares/lvgl)


# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl
    # Add user defined libraries
)
