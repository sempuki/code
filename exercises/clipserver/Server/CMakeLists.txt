#
# Clipbook Server Prototype CMake build configuration
#

cmake_minimum_required (VERSION 2.8)

project (Server)

add_subdirectory(jsoncpp-src)
add_subdirectory(Server)
add_subdirectory(TestClient)
