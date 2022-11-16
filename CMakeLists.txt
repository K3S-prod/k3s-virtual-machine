project(k3s)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(K3S_BINARY_DIR ${CMAKE_BINARY_DIR})

add_subdirectory(assembler)
