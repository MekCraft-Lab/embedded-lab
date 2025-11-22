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
set(CMAKE_PROJECT_NAME VGT6-ShellTest)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(cmake/gcc-arm-none-eabi.cmake)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        Drivers/Peripheral/GPIO/gpio-intf.hpp
        Drivers/Peripheral/GPIO/gpio-exit-decorator.cpp
        Drivers/Peripheral/GPIO/gpio-reg-impl.cpp
        Drivers/Peripheral/GPIO/gpio-lib-impl.cpp
        Core/Src/freertos.cpp
        Applications/app-intf.h
        Applications/app-shell.cpp
        Drivers/Communications/comm-intf.h
        Drivers/Communications/comm-uart-impl.cpp
        Drivers/Communications/comm-cdc-impl.cpp
        Middlewares/Third_Party/LetterShell/shell.c
        Middlewares/Third_Party/LetterShell/shell_cmd_list.c
        Middlewares/Third_Party/LetterShell/shell_companion.c
        Middlewares/Third_Party/LetterShell/shell_ext.c
        Middlewares/Third_Party/LetterShell/shell_cpp.h
        Middlewares/Third_Party/LetterShell/log/log.h
        Middlewares/Third_Party/LetterShell/log.c
        Applications/app-file.cpp
        Middlewares/Third_Party/LetterShell/game.c
        Middlewares/Third_Party/LetterShell/shell_cmd_group.c
        Middlewares/Third_Party/LetterShell/shell_passthrough.c
        Middlewares/Third_Party/LetterShell/shell_secure_user.c
        Middlewares/Third_Party/LetterShell/2048.c
        Middlewares/Third_Party/LetterShell/pushbox.c
        Drivers/Communications/comm-ospi.cpp
        Drivers/Services/serv-time.cpp
        Drivers/Services/serv-time.h
        Drivers/Devices/w25qxx.cpp
        Drivers/Devices/w25qxx.h
        Drivers/Devices/w25qxx-comm-adpter.cpp
        Drivers/Communications/comm-ospi.h
        Applications/app-stackOverFlow.cpp
        Drivers/Devices/w25qxx-inst.h
        Middlewares/Third_Party/LittleFs/lfs.c
        Middlewares/Third_Party/LittleFs/lfs_util.c
        Adapters/adapter-lfs.cpp
        Adapters/adapter-lfs.h
        Applications/ShellCmd/cmd-normal.cpp
        Applications/ShellCmd/cmd-normal.h
        Applications/app-shell.h
        Applications/ShellCmd/cmd-task.cpp
        Applications/ShellCmd/cmd-task.h
        Applications/app-file.h
        Applications/ShellCmd/cmd-file.h
        Applications/ShellCmd/cmd-file.cpp
        Applications/app-console.h
        Applications/app-console.cpp
        Applications/Utils/ringbuffer.h
        Applications/Utils/ringBuffer.cpp
)

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
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
