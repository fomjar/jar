cmake_minimum_required(VERSION 3.0.0)
project(jar VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB_RECURSE SRCS "src/*.cpp" "src/*.c")
file(GLOB_RECURSE INCS "src/*.hpp" "src/*.h")
include_directories(../src)

# lib
add_library(${PROJECT_NAME} SHARED ${SRCS})

# test
add_executable(${PROJECT_NAME}_test test/test.cpp ${SRCS})

