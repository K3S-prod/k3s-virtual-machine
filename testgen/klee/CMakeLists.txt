
add_executable(klee_sample
    main.cpp
)


link_directories(AFTER )
target_link_libraries(klee_sample 
    encoder
    runtime
    gc
)