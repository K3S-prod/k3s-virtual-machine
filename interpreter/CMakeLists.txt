set(INTERPRETER_BINARY_DIR ${K3S_BINARY_DIR}/interpreter)
set(INTERPRETER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/interpreter)
set(ISA "${CMAKE_SOURCE_DIR}/isa/isa.yaml")

set(TEMPLATES_GENERATOR "${CMAKE_SOURCE_DIR}/isa/isa_parser.rb")
set(TEMPLATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set(GENERATED_DIR "${INTERPRETER_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(TEMPLATES_NAMES
    "dispatch_table_single.inl"
    "dispatch_table.inl"
    "inst_decoder.h"
    "inst_decoder.cpp"
    "opcodes.h"
    "reg_types.inl"
)

list(TRANSFORM TEMPLATES_NAMES PREPEND "${TEMPLATES_DIR}/" OUTPUT_VARIABLE TEMPLATES)
list(TRANSFORM TEMPLATES APPEND ".erb")
list(TRANSFORM TEMPLATES_NAMES PREPEND "${GENERATED_DIR}/" OUTPUT_VARIABLE TEMPLATES_INSTANTIATED)

if (TRACE)
    add_definitions(-DTRACE)
endif()

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

macro(make_interpreter_lib target)
    add_library(${target}
    ${INTERPRETER_SRC}
    )

    add_dependencies(${target} interpreter_gen)

    target_compile_options(${target} PUBLIC -ggdb3)
    target_link_libraries(${target}
    classfile
    )
    target_include_directories(${target} PUBLIC ${INTERPRETER_BINARY_DIR})
    target_include_directories(${target} PUBLIC ${INTERPRETER_SOURCE_DIR})
endmacro()

make_interpreter_lib(interpreter)
make_interpreter_lib(interpreter_hooks)
target_compile_definitions(interpreter_hooks PUBLIC K3S_FETCH_HOOK)
make_interpreter_lib(interpreter_klee)
target_compile_options(interpreter_hooks PUBLIC ${CMAKE_C_FLAGS} -emit-llvm)