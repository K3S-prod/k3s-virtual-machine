#ifndef ALLOCATOR_ALLOCATOR_H
#define ALLOCATOR_ALLOCATOR_H

#include "interpreter/register.h"
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
private:
    std::vector<std::vector<uint8_t>> storage_ {};
};

}  // namespace k3s

#endif  // ALLOCATOR_ALLOCATOR_H
