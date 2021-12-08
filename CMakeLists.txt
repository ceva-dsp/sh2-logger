#
# FTDI dsf logger
#
cmake_minimum_required(VERSION 2.8)
project(sh2_logger)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS "-std=gnu99")
set(CMAKE_CXX_STANDARD 11)

#build hal lib
# add_subdirectory(sh2-ftdi-hal)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2-ftdi-hal")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/hal")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/tclap/include")

#link_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2-ftdi-hal")

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
    #    firmware-bno.c
    #    firmware-fsp200.c
    HcBinFile.cpp
    sh2/sh2.c
    sh2/sh2_SensorValue.c
    sh2/sh2_util.c
    sh2/shtp.c
    )

# Ooof.  Trying to eliminate sh2-ftdi-hal but the libraries are in there.
if(WIN32)
	if(CMAKE_CL_64)
		set(LIBRARIES ${LIBRARIES} sh2-ftdi-hal/x64/ftd2xx)
	else()
		set(LIBRARIES ${LIBRARIES} sh2-ftdi-hal/Win32/ftd2xx)
	endif()
endif()

	
# Link 
target_link_libraries(sh2_logger ${LIBRARIES})

# Install docs, license, sample config
install(FILES
    README.md
    LICENSE
    DESTINATION .)

