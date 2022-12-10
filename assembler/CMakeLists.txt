
set(ASSEMBLER_BINARY_DIR ${K3S_BINARY_DIR}/assembler)
set(ASSEMBLER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assembler)
set(ISA "${CMAKE_SOURCE_DIR}/isa/isa.yaml")
set(TEMPLATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set(GENERATED_DIR "${ASSEMBLER_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
set(TEMPLATES_GENERATOR "${CMAKE_SOURCE_DIR}/isa/isa_parser.rb")

set(LEXER "lexer.l")
set(GRAMMAR "grammar.l")
set(OPCODES "opcodes.h")
set(TEMPLATES_LIST "${TEMPLATES_DIR}/${LEXER}.erb" "${TEMPLATES_DIR}/${GRAMMAR}.erb" "${TEMPLATES_DIR}/${OPCODES}.erb")
set(TEMPLATES_INSTANTIATED "${GENERATED_DIR}/${LEXER}" "${GENERATED_DIR}/${GRAMMAR}" "${GENERATED_DIR}/${OPCODES}")

add_custom_command(
    OUTPUT ${TEMPLATES_INSTANTIATED}
    DEPENDS ${TEMPLATES_GENERATOR} ${ISA} ${TEMPLATES_LIST} 
    COMMAND ruby ${TEMPLATES_GENERATOR} ${ISA} ${TEMPLATES_LIST} ${TEMPLATES_INSTANTIATED}
)
add_custom_target(assembler_gen DEPENDS ${TEMPLATES_INSTANTIATED})

add_custom_command(
    OUTPUT "${GENERATED_DIR}/${LEXER}.cpp" "${GENERATED_DIR}/${GRAMMAR}.cpp" "${GENERATED_DIR}/${GRAMMAR}.hpp"
    DEPENDS assembler_gen
    COMMAND flex "-o" "${GENERATED_DIR}/${LEXER}.cpp" "${GENERATED_DIR}/${LEXER}"
    COMMAND bison "-o" "${GENERATED_DIR}/${GRAMMAR}.cpp" -d "${GENERATED_DIR}/${GRAMMAR}"
)

set(ASSEMBLER_SRC 
    "${GENERATED_DIR}/${LEXER}.cpp"
    "${GENERATED_DIR}/${GRAMMAR}.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/assembler.cpp"
)
add_executable(assembler
    ${ASSEMBLER_SRC}
)

target_compile_options(assembler PUBLIC -ggdb3)
target_link_libraries(assembler fl y)
target_include_directories(assembler PUBLIC ${ASSEMBLER_BINARY_DIR})
target_include_directories(assembler PUBLIC ${ASSEMBLER_SOURCE_DIR})
