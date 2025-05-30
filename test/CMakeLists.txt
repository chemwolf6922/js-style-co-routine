cmake_minimum_required(VERSION 3.10)

# Set the project name
project(test)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE "Debug")

add_compile_options(
    -Wall
    -Wextra
    -Werror)

# Add the executable

add_executable(TestPromise
    TestPromise.cpp)

target_link_libraries(TestPromise
    tev-cpp)

add_executable(TestAsyncGenerator
    TestAsyncGenerator.cpp)

target_link_libraries(TestAsyncGenerator
    tev-cpp)
