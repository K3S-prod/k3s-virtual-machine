#ifndef ALLOCATOR_ALLOCATOR_H
#define ALLOCATOR_ALLOCATOR_H

#include "interpreter/register.h"
#include "interpreter/types/coretypes.h"
#include <vector>
#include <cstdint>
#include <new>

namespace k3s {

using Payload = Register;

class Allocator {
public:
    void *Alloc(size_t n_elements)
    {
        storage_.emplace_back(n_elements * sizeof(Payload));
        return storage_.back().data();
    }
    
    void *AllocBytes(size_t n_bytes)
    {
        storage_.emplace_back(n_bytes);
        return storage_.back().data();
    }
    
    coretypes::Function *AllocFunction(size_t bc_offs)
    {
        void *ptr = AllocBytes(sizeof(coretypes::Function));
        return new (ptr) coretypes::Function(bc_offs);
    }
    coretypes::Array *AllocArray(size_t size)
    {
        void *ptr = AllocBytes(sizeof(coretypes::Array));
        return new (ptr) coretypes::Array(size);
    }
private:
    std::vector<std::vector<uint8_t>> storage_ {};
};

}  // namespace k3s

#endif  // ALLOCATOR_ALLOCATOR_H
