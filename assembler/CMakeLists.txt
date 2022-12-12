
set(ASSEMBLER_BINARY_DIR ${K3S_BINARY_DIR}/assembler)
set(ASSEMBLER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assembler)
set(ISA "${CMAKE_SOURCE_DIR}/isa/isa.yaml")
set(TEMPLATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set(GENERATED_DIR "${ASSEMBLER_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
set(TEMPLATES_GENERATOR "${CMAKE_SOURCE_DIR}/isa/isa_parser.rb")

set(LEXER "lexer.l")
set(GRAMMAR "grammar.l")

set(TEMPLATES_NAMES
    ${GRAMMAR}
    ${LEXER}
)

list(TRANSFORM TEMPLATES_NAMES PREPEND "${TEMPLATES_DIR}/" OUTPUT_VARIABLE TEMPLATES)
list(TRANSFORM TEMPLATES APPEND ".erb")
list(TRANSFORM TEMPLATES_NAMES PREPEND "${GENERATED_DIR}/" OUTPUT_VARIABLE TEMPLATES_INSTANTIATED)

add_custom_command(
    OUTPUT ${TEMPLATES_INSTANTIATED}
    DEPENDS ${TEMPLATES_GENERATOR} ${ISA} ${TEMPLATES} 
    COMMAND ruby ${TEMPLATES_GENERATOR} ${ISA} ${TEMPLATES} ${TEMPLATES_INSTANTIATED}
)
add_custom_target(assembler_gen DEPENDS ${TEMPLATES_INSTANTIATED})

add_custom_command(
    OUTPUT "${GENERATED_DIR}/${LEXER}.cpp" "${GENERATED_DIR}/${GRAMMAR}.cpp" "${GENERATED_DIR}/${GRAMMAR}.hpp"
    DEPENDS assembler_gen interpreter_gen ${TEMPLATES_INSTANTIATED}
    COMMAND flex "-o" "${GENERATED_DIR}/${LEXER}.cpp" "${GENERATED_DIR}/${LEXER}"
    COMMAND bison "-o" "${GENERATED_DIR}/${GRAMMAR}.cpp" -v -d "${GENERATED_DIR}/${GRAMMAR}"
)

set(ASSEMBLER_LIB_SRC
    "${GENERATED_DIR}/${LEXER}.cpp"
    "${GENERATED_DIR}/${GRAMMAR}.cpp"
)

add_library(assembler
    ${ASSEMBLER_LIB_SRC}
)

add_executable(asm
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

target_compile_options(assembler PUBLIC -ggdb3)
target_link_libraries(assembler fl y)
target_link_libraries(asm assembler)
