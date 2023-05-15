
cmake_minimum_required(VERSION 3.16)

project(k3s)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(K3S_BINARY_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(${K3S_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(assembler)
add_subdirectory(classfile)
add_subdirectory(allocator)
add_subdirectory(interpreter)
add_subdirectory(runtime)
