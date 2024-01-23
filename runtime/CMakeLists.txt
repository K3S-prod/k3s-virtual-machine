add_library(runtime
    runtime.cpp
)

add_executable(k3s
    main.cpp
)

target_link_libraries(k3s
    runtime
    interpreter
    gc
)
