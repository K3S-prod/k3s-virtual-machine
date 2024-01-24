add_custom_target(asm_tests DEPENDS asm)
add_custom_target(run_tests DEPENDS k3s asm)
add_custom_target(cov_tests)

file(GLOB COV_TESTS ${CMAKE_SOURCE_DIR}/tests/*.k3s)

foreach(TEST ${COV_TESTS})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    set(ASM_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/asm_tests/${TEST_NAME}.k3sm)

    add_custom_command(
        OUTPUT ${ASM_OUTPUT}
        COMMAND ${BIN}/asm ${TEST} ${ASM_OUTPUT}
        DEPENDS ${BIN}/asm ${TEST}
    )
    add_custom_target(asm_test_${TEST_NAME} DEPENDS ${ASM_OUTPUT})

    set(PREFIX "")
    if (TRACE)
        set(TRACE_PATH coverage/${TEST_NAME}.ktrace)
        set(PREFIX K3S_TRACE=${TRACE_PATH} ${PREFIX})
    endif()
    if (COVERAGE)
        set(COVERAGE_PATH coverage/${TEST_NAME}.raw)

        set(PREFIX LLVM_PROFILE_FILE=${COVERAGE_PATH} ${PREFIX})

        add_custom_command(
            OUTPUT ${COVERAGE_PATH}
            COMMAND echo "Start ${TEST_NAME}"
            COMMAND ${PREFIX} ${BIN}/k3s ${ASM_OUTPUT} > ${TEST_NAME}.log 2>&1
            COMMAND echo "Finish ${TEST_NAME}"
            DEPENDS ${ASM_OUTPUT} ${BIN}/k3s
        )
        add_custom_target(run_test_${TEST_NAME} DEPENDS ${COVERAGE_PATH})
    else()
        add_custom_target(run_test_${TEST_NAME}
            COMMAND echo "Start ${TEST_NAME}"
            COMMAND ${PREFIX} ${BIN}/k3s ${ASM_OUTPUT} > ${TEST_NAME}.log 2>&1
            COMMAND echo "Finish ${TEST_NAME}"
            DEPENDS ${ASM_OUTPUT} ${BIN}/k3s
        )
        add_dependencies(run_tests run_test_${TEST_NAME})
    endif()
    
endforeach()

if (COVERAGE)
    foreach(COV_RAW ${COV_TESTS})
        get_filename_component(TEST_NAME ${COV_RAW} NAME_WE)

        add_custom_command(
            OUTPUT coverage/${TEST_NAME}.profdata
            COMMAND llvm-profdata merge -sparse coverage/${TEST_NAME}.raw -o coverage/${TEST_NAME}.profdata
            DEPENDS run_tests run_test_${TEST_NAME}
        )
        add_custom_target(cov_${TEST_NAME} DEPENDS coverage/${TEST_NAME}.profdata)

        add_dependencies(cov_tests cov_${TEST_NAME})
    endforeach()
endif()
