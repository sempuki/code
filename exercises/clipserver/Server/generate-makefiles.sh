#!/bin/bash
# 
# Argument to generator should be "Release" or "Debug"

cmake -G "Unix Makefiles" -DCMAKE_CXX_COMPILER='/usr/bin/g++-4.8' -DCMAKE_BUILD_TYPE=${1}
