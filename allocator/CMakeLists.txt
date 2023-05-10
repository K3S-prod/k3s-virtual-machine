
set(ALLOCATOR_SOURCE_DIR ${CMAKE_SOURCE_DIR}/allocator)

set(ALLOCATOR_SRC 
    "${ALLOCATOR_SOURCE_DIR}/allocator.cpp"
)

add_library(allocator
    ${ALLOCATOR_SRC}
)

target_include_directories(allocator PUBLIC ${ALLOCATOR_SOURCE_DIR})
