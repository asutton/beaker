project(beaker CXX)

cmake_minimum_required(VERSION 3.15)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall")
# set(CMAKE_CXX_FLAGS "-fsanitize=address -fsanitize=undefined -Wall")
include_directories(.)

add_subdirectory(beaker)
