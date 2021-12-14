cmake_minimum_required(VERSION 3.4)
project(sh2_logger)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS "-std=gnu99")
set(CMAKE_CXX_STANDARD 11)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/hal")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/tclap/include")


if(WIN32)
set(PLATFORM_CODE Win)
else()
set(PLATFORM_CODE Linux)
endif()

# Build 
add_executable(sh2_logger
    sh2_logger.cpp
    LoggerApp.cpp
    DsfLogger.cpp
	ConsoleLogger.cpp
    FspDfu.cpp
    BnoDfu.cpp
    hal/bno_dfu_hal_${PLATFORM_CODE}.c
    hal/ftdi_hal_${PLATFORM_CODE}.c
    #TODO: uncomment when integrated
    #    firmware-bno.c
    #    firmware-fsp200.c
    HcBinFile.cpp
    sh2/sh2.c
    sh2/sh2_SensorValue.c
    sh2/sh2_util.c
    sh2/shtp.c
    )

# # Should be possible to group the libraries under directories like
#   lib/x64 and lib/Win32 and some combination of
#   CMAKE_PREFIX_PATH/CMAKE_LIBRARY_PATH
# find_library(FTD2XX_LIB ftd2xx ${CMAKE_SOURCE_DIR}/sh2-ftdi-hal)
if(WIN32)
    if(CMAKE_CL_64)
         find_library(FTD2XX_LIB ftd2xx ${CMAKE_SOURCE_DIR}/sh2-ftdi-hal/x64)
    else()
         find_library(FTD2XX_LIB ftd2xx ${CMAKE_SOURCE_DIR}/sh2-ftdi-hal/Win32)
    endif()
    target_link_libraries(sh2_logger ${FTD2XX_LIB})
endif()


# Install docs, license, sample config
install(FILES
    README.md
    LICENSE
    DESTINATION .)

