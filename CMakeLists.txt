
cmake_minimum_required(VERSION 3.16)

if (TOOLCHAIN STREQUAL "CLANG")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    if (COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -g -O0 -Xclang -disable-O0-optnone")

endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")

project(k3s)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(BIN ${CMAKE_BINARY_DIR}/bin)
set(K3S_BINARY_DIR ${CMAKE_BINARY_DIR})

include_directories(${K3S_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(/home/linuxbrew/.linuxbrew/include)

add_subdirectory(assembler)
add_subdirectory(tests)
add_subdirectory(classfile)
add_subdirectory(allocator)
add_subdirectory(interpreter)
add_subdirectory(runtime)
add_subdirectory(testgen)
