#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

#include <cassert>
#include <cstring>
#include <iostream>

#define ASSERT(x) assert(x)
#define LOG_FATAL(component, msg) \
{                                                                       \
    std::cerr << "[" << #component << "] FATAL: " << msg << std::endl;  \
    std::abort();                                                       \
    __builtin_unreachable();                                            \
}

#ifndef NDEBUG
#define LOG_DEBUG(component, msg) \
{                                                                \
    std::cerr << "[" << #component << "] " << msg << std::endl;  \
}
#else
#define LOG_DEBUG(component, msg)
#endif  // NDEBUG


#define BIT_CAST(type, var) \

template <typename T1, typename T2>
T1 bit_cast(T2 val) {
    ASSERT(sizeof(T2) == sizeof(T1));
    T1 temp;
    std::memcpy(&temp, &val, sizeof(T2));
    return temp;
}


#endif  // COMMON_MACRO_H
