set(CLASSFILE_BINARY_DIR ${K3S_BINARY_DIR}/classfile)
set(CLASSFILE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/classfile)

add_library(classfile class_file.cpp)
target_link_libraries(classfile assembler)
target_compile_options(classfile PUBLIC -ggdb3)