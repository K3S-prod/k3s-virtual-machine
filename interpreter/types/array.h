#ifndef INTERPRETER_TYPES_ARRAY_H
#define INTERPRETER_TYPES_ARRAY_H

#include "interpreter/register.h"

#include <cstddef>
#include <vector>

namespace k3s::coretypes {

class Array {
public:
    Array(size_t size) : data_(size) {}
    Register &GetElem(size_t idx) { 
        return data_.at(idx); 
    }
    void SetElem(size_t idx, const Register &val) { data_.at(idx) = val; }
private:
    std::vector<Register> data_;
};

}

#endif