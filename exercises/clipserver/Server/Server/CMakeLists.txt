#
# Clipbook Server Prototype CMake build configuration
#

project(jsoncpp)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

set (JSONCPP_INCLUDE_PATH "../jsoncpp-src/include")
set (JSONCPP_LIB_PATH "../jsoncpp-src/lib")

if (UNIX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
	SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif ()

add_executable(Server 
	"server.cpp" 
	"message.cpp" 
	"socket.cpp" 
	"util.cpp")

include_directories(${JSONCPP_INCLUDE_PATH})

find_library(JSONCPP_LIBRARY "jsoncpp" PATHS ${JSONCPP_LIB_PATH})
target_link_libraries(Server ${JSONCPP_LIBRARY})
