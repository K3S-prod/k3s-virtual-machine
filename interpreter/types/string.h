#ifndef INTERPRETER_TYPES_STRING_H
#define INTERPRETER_TYPES_STRING_H

#include "allocator/containers.h"

namespace k3s::coretypes {

class String {
public:
    using elem_t = char;

    String(size_t size, const char *c_str) : size_(size)
    {
        memcpy(data_, c_str, size + 1);
        ASSERT(data_[size] == '\0');
    }

    auto *GetData() {
        return &data_[0]; 
    }
    auto GetSize() {
        return size_;
    }
private:
    size_t size_;
    elem_t data_[];
};

}

#endif
