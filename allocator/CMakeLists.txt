add_library(gc
    gc_region.cpp
)

target_compile_options(gc PUBLIC -ggdb3)
