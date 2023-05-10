#ifndef INTERPRETER_TYPES_ARRAY_H
#define INTERPRETER_TYPES_ARRAY_H

#include "interpreter/register.h"

#include <cstddef>
#include <vector>

namespace k3s::coretypes {

class Array {
public:
    using elem_t = Register;

    Array(size_t size) : size_(size) {}
    Register &GetElem(size_t idx) { 
        return data_[idx]; 
    }
    void SetElem(size_t idx, const Register &val) { data_[idx] = val; }
private:
    size_t size_;
    elem_t data_[];
};

}

#endif
