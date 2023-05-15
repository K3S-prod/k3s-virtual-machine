set(INTERPRETER_BINARY_DIR ${K3S_BINARY_DIR}/interpreter)
set(INTERPRETER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/interpreter)
set(ISA "${CMAKE_SOURCE_DIR}/isa/isa.yaml")

set(TEMPLATES_GENERATOR "${CMAKE_SOURCE_DIR}/isa/isa_parser.rb")
set(TEMPLATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set(GENERATED_DIR "${INTERPRETER_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(TEMPLATES_NAMES
    "dispatch_table.inl"
    "inst_decoder.h"
    "inst_decoder.cpp"
    "opcodes.h"
    "reg_types.inl"
)

list(TRANSFORM TEMPLATES_NAMES PREPEND "${TEMPLATES_DIR}/" OUTPUT_VARIABLE TEMPLATES)
list(TRANSFORM TEMPLATES APPEND ".erb")
list(TRANSFORM TEMPLATES_NAMES PREPEND "${GENERATED_DIR}/" OUTPUT_VARIABLE TEMPLATES_INSTANTIATED)

add_custom_command(
    OUTPUT ${TEMPLATES_INSTANTIATED}
    DEPENDS ${TEMPLATES_GENERATOR} ${ISA} ${TEMPLATES} 
    COMMAND ruby ${TEMPLATES_GENERATOR} ${ISA} ${TEMPLATES} ${TEMPLATES_INSTANTIATED}
)

add_custom_target(interpreter_gen DEPENDS ${TEMPLATES_INSTANTIATED})

set(INTERPRETER_SRC 
    "${INTERPRETER_SOURCE_DIR}/interpreter.cpp"
    "${INTERPRETER_SOURCE_DIR}/register.cpp"
    "${INTERPRETER_BINARY_DIR}/generated/inst_decoder.cpp"
)

add_library(interpreter
    ${INTERPRETER_SRC}
)

add_dependencies(interpreter interpreter_gen)

target_compile_options(interpreter PUBLIC -ggdb3)
target_link_libraries(interpreter
    classfile
)
target_include_directories(interpreter PUBLIC ${INTERPRETER_BINARY_DIR})
target_include_directories(interpreter PUBLIC ${INTERPRETER_SOURCE_DIR})
