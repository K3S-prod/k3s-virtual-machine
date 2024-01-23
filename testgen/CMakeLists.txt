
add_executable(testgen
    main.cpp
    runtime_image.cpp
)

target_compile_definitions(testgen PUBLIC K3S_FETCH_HOOK)

target_link_libraries(testgen
    assembler
    runtime
    interpreter_hooks
    gc
)
