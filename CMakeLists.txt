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
add_subdirectory(sh2-ftdi-hal)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2-ftdi-hal")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/sh2-ftdi-hal")

# Build 
add_executable(sh2_logger
    sh2_logger.cpp
    LoggerApp.cpp
    DsfLogger.cpp
	ConsoleLogger.cpp
    FspDfu.cpp
    BnoDfu.cpp
    )
	
# Link 
target_link_libraries(sh2_logger sh2_ftdi_hal)

# Install docs, license, sample config
install(FILES
    README.md
    LICENSE
    DESTINATION .)

