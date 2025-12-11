# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/env/Espressif/frameworks/esp-idf-v5.5.1/components/bootloader/subproject")
  file(MAKE_DIRECTORY "C:/env/Espressif/frameworks/esp-idf-v5.5.1/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader"
  "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix"
  "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix/tmp"
  "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix/src/bootloader-stamp"
  "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix/src"
  "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Program/MekCraft-Labs/embedded-lab/esp32/play-station/cmake-build-debug-espidfautogen40845367/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
