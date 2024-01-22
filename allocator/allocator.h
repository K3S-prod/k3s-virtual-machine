#ifndef ALLOCATOR_ALLOCATOR_H
#define ALLOCATOR_ALLOCATOR_H

#include "allocator/region.h"
#include "allocator/gc_region.h"
#include <sys/mman.h>
#include <cstdint>
#include <new>

namespace k3s {

class Allocator
{
    static constexpr uintptr_t ALLOC_START_ADDR = 0xE000000;
    static constexpr size_t ALLOC_SIZE = 1024 * 1024 * 32;
    static constexpr size_t GC_INTERNALS_SIZE = ALLOC_SIZE / 8;
    static constexpr size_t STACK_SIZE = ALLOC_SIZE / 8;
public:
    using ConstRegionT = Region<ALLOC_START_ADDR, ALLOC_SIZE / 2 - GC_INTERNALS_SIZE - STACK_SIZE>;
    using GCInternalsRegionT = Region<ALLOC_START_ADDR + ALLOC_SIZE / 2 - GC_INTERNALS_SIZE - STACK_SIZE, GC_INTERNALS_SIZE>;
    using StackRegionT = Region<ALLOC_START_ADDR + ALLOC_SIZE / 2 - STACK_SIZE, STACK_SIZE>;
    using RuntimeRegionT = GCRegion<ALLOC_START_ADDR + ALLOC_SIZE / 2, ALLOC_SIZE / 2>;

    static void Init()
    {
        // void *buf = malloc(ALLOC_SIZE);
        void *buf = mmap(reinterpret_cast<void *>(ALLOC_START_ADDR), ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        ASSERT(buf == reinterpret_cast<void *>(ALLOC_START_ADDR));
        ConstRegionT::Reset();
        GCInternalsRegionT::Reset();
        StackRegionT::Reset();
        RuntimeRegionT::Reset();
    }

    static void Destroy()
    {
        munmap(reinterpret_cast<void *>(ALLOC_START_ADDR), ALLOC_SIZE);
    }

    auto &ConstRegion()
    {
        return const_region_;
    }

    auto &GcInternalsRegion()
    {
        return gc_internals_region_;
    }

    auto &StackRegion()
    {
        return stack_region_;
    }

    auto &ObjectsRegion()
    {
        return objects_region_;
    }

private:
    ConstRegionT const_region_;
    GCInternalsRegionT gc_internals_region_;
    StackRegionT stack_region_;
    RuntimeRegionT objects_region_;
};

}  // namespace k3s

#endif  // ALLOCATOR_ALLOCATOR_H
