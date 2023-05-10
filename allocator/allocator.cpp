#include "allocator.h"
#include "interpreter/types/coretypes.h"

namespace k3s {

template <uintptr_t START_PTR, size_t SIZE>
coretypes::Function *Allocator::Region<START_PTR, SIZE>::NewFunction(size_t bc_offs)
{
    void *ptr = AllocBytes(sizeof(coretypes::Function));
    return new (ptr) coretypes::Function(bc_offs);
}

template <uintptr_t START_PTR, size_t SIZE>
coretypes::Array *Allocator::Region<START_PTR, SIZE>::NewArray(size_t size)
{
    void *ptr = AllocBytes(sizeof(coretypes::Array) + sizeof(coretypes::Array::elem_t) * size);
    return new (ptr) coretypes::Array(size);
}

template <uintptr_t START_PTR, size_t SIZE>
coretypes::String *Allocator::Region<START_PTR, SIZE>::NewString(const char *c_str)
{
    size_t size = std::string_view(c_str).size();
    void *ptr = AllocBytes(sizeof(coretypes::String) + sizeof(coretypes::String::elem_t) * size + 1);
    return new (ptr) coretypes::String(size, c_str);
}

template coretypes::Function *Allocator::RuntimeRegionT::NewFunction(size_t bc_offs);
template coretypes::Array *Allocator::RuntimeRegionT::NewArray(size_t size);
template coretypes::String *Allocator::RuntimeRegionT::NewString(const char *c_str);
// class Allocator::Region<251658240ul, 16777216ul>;

}  // namespace k3s
