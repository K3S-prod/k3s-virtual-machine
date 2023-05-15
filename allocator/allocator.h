#ifndef ALLOCATOR_ALLOCATOR_H
#define ALLOCATOR_ALLOCATOR_H

#include "interpreter/register.h"
#include "interpreter/types/coretypes.h"
#include <sys/mman.h>
#include <vector>
#include <cstdint>
#include <new>

namespace k3s {


class Allocator
{
    static constexpr uintptr_t ALLOC_START_ADDR = 0xE000000;
    static constexpr uintptr_t ALLOC_SIZE = 1024 * 1024 * 32;

    template <uintptr_t START_PTR, size_t SIZE>
    class Region
    {
        template <typename T>
        class AllocatorRequirements
        {
        public:
            using value_type = T;

            [[nodiscard]] T* allocate(size_t n)
            {
                return Region::Alloc<T>(n);
            }
            void deallocate(T *ptr, size_t n)
            {
                return;
            }
        private:
            Region *region_{};
        };

    public:
        void Init()
        {
            *GetCursor() = sizeof(*GetCursor()); 
        }

        template <typename T>
        auto Adapter()
        {
            return AllocatorRequirements<T>();
        }
        template <typename T>
        static T *Alloc(size_t n_elems)
        {
            return reinterpret_cast<T *>(AllocBytes(n_elems * sizeof(T)));
        }

        static auto *GetCursor()
        {
            return reinterpret_cast<size_t *>(START_PTR);
        }
        
        static void *GetStartPtr()
        {
            return reinterpret_cast<void *>(START_PTR);
        }

        coretypes::Function *NewFunction(size_t bc_offs)
        {
            void *ptr = AllocBytes(sizeof(coretypes::Function));
            return new (ptr) coretypes::Function(bc_offs);
        }

        coretypes::Array *NewArray(size_t size)
        {
            void *ptr = AllocBytes(sizeof(coretypes::Array) + sizeof(coretypes::Array::elem_t) * size);
            return new (ptr) coretypes::Array(size);
        }

        static void *AllocBytes(size_t n_bytes)
        {
            size_t new_cursor = *GetCursor() + n_bytes;
            if (new_cursor > SIZE) {
                LOG_FATAL(ALLOCATOR, "OOM");
            }
            auto allocated = GetStartPtr() + *GetCursor();
            *GetCursor() = new_cursor;
            return allocated;
        }
    };

public:
    Allocator()
    {
        void *buf = mmap(reinterpret_cast<void *>(ALLOC_START_ADDR), ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        ASSERT(buf == reinterpret_cast<void *>(ALLOC_START_ADDR));
        const_region_.Init();
        runtime_region_.Init();
    }
    ~Allocator()
    {
        munmap(reinterpret_cast<void *>(ALLOC_START_ADDR), ALLOC_SIZE);
    }

    auto &ConstRegion()
    {
        return const_region_;
    }

    auto &RuntimeRegion()
    {
        return runtime_region_;
    }

private:
    Region<ALLOC_START_ADDR, ALLOC_SIZE / 2> const_region_;
    Region<ALLOC_START_ADDR + ALLOC_SIZE / 2, ALLOC_SIZE / 2> runtime_region_;
};

}  // namespace k3s

#endif  // ALLOCATOR_ALLOCATOR_H
