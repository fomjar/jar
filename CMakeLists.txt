cmake_minimum_required(VERSION 3.0.0)
project(jar VERSION 0.1.0)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB_RECURSE SRCS "include/*.cpp" "include/*.c")
message("SRCS=${SRCS}")
file(GLOB_RECURSE INCS "include/*.hpp" "include/*.h")
message("INCS=${INCS}")

# include_directories("include")

# lib
set(LIBRARY_OUTPUT_PATH "../lib")
add_library(${PROJECT_NAME} SHARED ${SRCS})
add_library(${PROJECT_NAME}_static STATIC ${SRCS})

# bin
set(EXECUTABLE_OUTPUT_PATH "../bin")
add_executable(${PROJECT_NAME}_test "test/test.cpp" ${SRCS})

