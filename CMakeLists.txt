cmake_minimum_required(VERSION 3.0.0)
project(jar VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(jar src/main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
