#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

#include <cassert>
#include <iostream>

#define ASSERT(x) assert(x)
#define LOG_FATAL(component, msg) \
{                                                                       \
    std::cerr << "[" << #component << "] FATAL:" << msg << std::endl;  \
    std::abort();                                                       \
    __builtin_unreachable();                                            \
}
#endif  // COMMON_MACRO_H
