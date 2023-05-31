
add_executable(k3s
    runtime.cpp
)

target_link_libraries(k3s
    interpreter
    gc
)
