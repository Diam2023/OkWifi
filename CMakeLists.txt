cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME OkWifi)
set(PROJECTNAME OkWifi)

# CXX 17 Support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug mode
set(CMAKE_BUILD_TYPE Debug)

# Project Version
set(PROJECT_VER 0.1)

# Build target is esp32c3
set(IDF_TARGET esp32c3)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(OkWifi)
