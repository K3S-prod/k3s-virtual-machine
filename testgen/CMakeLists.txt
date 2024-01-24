
add_executable(testgen
    main.cpp
    runtime_image.cpp
)

target_compile_definitions(testgen PUBLIC K3S_FETCH_HOOK)

include_directories(/usr/include/ruby-3.0.0/)
include_directories(/usr/include/x86_64-linux-gnu/ruby-3.0.0/)
target_link_libraries(testgen
    encoder
    runtime
    interpreter_hooks
    gc
    ruby-3.0
) 

add_subdirectory(klee)
