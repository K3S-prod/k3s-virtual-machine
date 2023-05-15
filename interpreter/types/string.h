#ifndef INTERPRETER_TYPES_STRING_H
#define INTERPRETER_TYPES_STRING_H

#include "allocator/object_header.h"

namespace k3s::coretypes {

class String : public ObjectHeader{
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
    
    template <uintptr_t START_PTR, size_t SIZE>
    static coretypes::String *New(GCRegion<START_PTR, SIZE> region, const char *c_str);
private:
    size_t size_;
    elem_t data_[];
};

template <uintptr_t START_PTR, size_t SIZE>
inline String *String::New(GCRegion<START_PTR, SIZE> region, const char *c_str)
{
    size_t size = std::string_view(c_str).size();
    size_t allocated_size = sizeof(coretypes::String) + sizeof(coretypes::String::elem_t) * size + 1;
    void *storage = region.AllocBytes(allocated_size);
    auto *ptr = new (storage) String(size, c_str);
    ptr->SetAllocatedSize(allocated_size);
    return ptr;
}
}

#endif
