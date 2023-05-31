#ifndef ALLOCATOR_REGION_H
#define ALLOCATOR_REGION_H

#include "common/macro.h"
#include <cstdint>
#include <cstddef>

namespace k3s {

template <uintptr_t START_PTR, size_t SIZE>
class Region
{
    template <typename T>
    class AllocatorRequirements
    {
    public:
        using value_type = T;
        AllocatorRequirements() = default;
        template <typename U>
        AllocatorRequirements(const AllocatorRequirements<U> &a2) {}
        template <typename U>
        bool operator==(AllocatorRequirements<U> &a2) const { return true; }
        [[nodiscard]] T* allocate(size_t n)
        {
            return Region::Alloc<T>(n);
        }
        void deallocate(T *ptr, size_t n)
        {
            return;
        }
    };
public:
    static bool Contains(const void *ptr)
    {
        auto intptr = reinterpret_cast<uintptr_t>(ptr);
        return (intptr >= START_PTR) && (intptr < (START_PTR + SIZE));
    }

    static void Reset()
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
    static char *GetStartPtr()
    {
        return reinterpret_cast<char *>(START_PTR);
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
    static size_t GetRemainingSpace()
    {
        return SIZE - *GetCursor();
    }
    static constexpr size_t MAX_ALLOC_SIZE = SIZE - sizeof(*GetCursor());
};

} // namespace k3s

#endif  // ALLOCATOR_REGION_H
