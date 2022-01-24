# Copyright 2018-22 CEVA, Inc.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License and
# any applicable agreements you may have with CEVA, Inc.
# You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.4)
project(sh2_logger VERSION 1.1.0)

configure_file(config.h.in config.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS "-std=gnu99")
set(CMAKE_CXX_STANDARD 11)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/hal")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ftdi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/tclap/include")
include_directories(${PROJECT_BINARY_BIN})  # allows generated config.h to be accessed.


if(WIN32)
  set(BNO_DFU_HAL hal/bno_dfu_hal_Win.c)
else()
  set(BNO_DFU_HAL hal/bno_dfu_hal_Linux.c)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
#Disable this warning, no mixing of gcc versions in this build
set(CMAKE_CXX_FLAGS "-Wno-psabi")
endif()

# Build 
add_executable(sh2_logger
    sh2_logger.cpp
    LoggerApp.cpp
    DsfLogger.cpp
    FspDfu.cpp
    BnoDfu.cpp
    hal/ftdi_hal.c
    ${BNO_DFU_HAL}
    HcBinFile.cpp
    sh2/sh2.c
    sh2/sh2_SensorValue.c
    sh2/sh2_util.c
    sh2/shtp.c
    WheelSource.cpp
    FileWheelSource.cpp
    )

if(WIN32)
    if(CMAKE_CL_64)
         find_library(FTD2XX_LIB ftd2xx ${CMAKE_SOURCE_DIR}/ftdi/x64)
    else()
         find_library(FTD2XX_LIB ftd2xx ${CMAKE_SOURCE_DIR}/ftdi/Win32)
    endif()
    target_link_libraries(sh2_logger ${FTD2XX_LIB})
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(sh2_logger pthread)
endif()

# Install docs, license, sample configs, and binary
install(FILES
    README.md
    LICENSE
    dr100.json
    6agUncalibratedEnu.json
    9agmRawCalibrated.json
    DESTINATION .)
install(TARGETS sh2_logger DESTINATION bin)
