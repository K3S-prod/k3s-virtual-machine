#ifndef INTERPRETER_TYPES_ARRAY_H
#define INTERPRETER_TYPES_ARRAY_H

#include "allocator/object_header.h"
#include "interpreter/register.h"

#include <cstddef>

namespace k3s::coretypes {

class Array : public ObjectHeader
{
public:
    using elem_t = Register;

    Array(size_t size) : size_(size)
    {
        for (size_t i = 0; i < size; i++) {
            GetElem(i)->Reset();
        }
    }
    Register *GetElem(size_t idx)
    { 
        return &data_[idx]; 
    }
    void SetElem(size_t idx, const Register &val) {
        ASSERT(idx < size_);
        data_[idx] = val;
    }

    template <uintptr_t START_PTR, size_t SIZE>
    static coretypes::Array *New(GCRegion<START_PTR, SIZE> region, size_t size);

    auto GetSize() const
    {
        return size_;
    }

private:
    size_t size_;
    elem_t data_[];
};


template <uintptr_t START_PTR, size_t SIZE>
inline coretypes::Array *Array::New(GCRegion<START_PTR, SIZE> region, size_t size)
{
    size_t allocated_size = sizeof(coretypes::Array) + sizeof(coretypes::Array::elem_t) * size;
    void *storage = region.AllocBytes(allocated_size);
    auto *ptr = new (storage) coretypes::Array(size);
    ptr->SetAllocatedSize(allocated_size);
    return ptr;
}

}

#endif
