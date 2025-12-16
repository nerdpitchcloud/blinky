cmake_minimum_required(VERSION 3.14)
project(blinky VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_AGENT "Build agent" ON)
option(BUILD_COLLECTOR "Build collector" ON)
option(BUILD_TESTS "Build tests" OFF)

add_subdirectory(shared)

if(BUILD_AGENT)
    add_subdirectory(agent)
endif()

if(BUILD_COLLECTOR)
    add_subdirectory(collector)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
