#ifndef INTERPRETER_TYPES_STRING_H
#define INTERPRETER_TYPES_STRING_H

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
    
    template <uintptr_t START_PTR, size_t SIZE>
    static coretypes::String *New(Allocator::Region<START_PTR, SIZE> region, const char *c_str);
private:
    size_t size_;
    elem_t data_[];
};

template <uintptr_t START_PTR, size_t SIZE>
inline String *String::New(Allocator::Region<START_PTR, SIZE> region, const char *c_str)
{
    size_t size = std::string_view(c_str).size();
    void *ptr = region.AllocBytes(sizeof(String) + sizeof(String::elem_t) * size + 1);
    return new (ptr) String(size, c_str);
}
}

#endif
