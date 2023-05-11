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

    template <uintptr_t START_PTR, size_t SIZE>
    static coretypes::Array *New(Allocator::Region<START_PTR, SIZE> region, size_t size);

private:
    size_t size_;
    elem_t data_[];
};


template <uintptr_t START_PTR, size_t SIZE>
inline coretypes::Array *Array::New(Allocator::Region<START_PTR, SIZE> region, size_t size)
{
    void *ptr = region.AllocBytes(sizeof(coretypes::Array) + sizeof(coretypes::Array::elem_t) * size);
    return new (ptr) coretypes::Array(size);
}

}

#endif
