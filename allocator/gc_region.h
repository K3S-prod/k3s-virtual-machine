#ifndef ALLOCATOR_GC_REGION_H
#define ALLOCATOR_GC_REGION_H

#include "allocator/region.h"
#include "interpreter/register.h"

namespace k3s {

template <uintptr_t START_PTR, size_t REGION_SIZE, size_t N_REGIONS, size_t REMAINING_SIZE>
class RegionsPool
{
public:
    static constexpr size_t MAX_ALLOC_SIZE = REGION_SIZE;
    static_assert(REGION_SIZE != 0);
    static_assert(REMAINING_SIZE >= N_REGIONS * REGION_SIZE);

    static void Reset()
    {
        decltype(this_)::Reset();
        decltype(others_)::Reset();
    }
    static auto GetThis() 
    {
        return decltype(this_)();
    }
    static auto GetOthers() 
    {
        return decltype(others_)();
    }

    static void *AllocBytes(size_t n_bytes)
    {
        if (decltype(this_)::GetRemainingSpace() >= n_bytes) {
            return decltype(this_)::AllocBytes(n_bytes);
        } else {
            CleanupThis();
            ASSERT(decltype(this_)::GetRemainingSpace() == decltype(this_)::MAX_ALLOC_SIZE);
            return decltype(this_)::AllocBytes(n_bytes);
        }
    }

    static void CleanupThis();

    static void MarkAndFetchTargetObjects();
    static bool MarkAndFetchRecursively(Register vreg);
    static void MarkArray(ObjectHeader *obj);
    static void MarkObject(ObjectHeader *obj);
    static void MarkFunction(ObjectHeader *obj);

    static void MoveObjects();
    static void RebindLinks();
    
    constexpr auto GetLastRegion()
    {
        return others_.GetLastRegion();
    }
    
    constexpr auto GetNextRegion()
    {
        return others_.GetThis();
    }

private:
    Region<START_PTR, REGION_SIZE> this_;
    RegionsPool<START_PTR + REGION_SIZE, REGION_SIZE, N_REGIONS - 1, REMAINING_SIZE - REGION_SIZE> others_;
};

// Last element (tenured space):
template <uintptr_t START_PTR, size_t REGION_SIZE, size_t REMAINING_SIZE>
class RegionsPool<START_PTR, REGION_SIZE, 0, REMAINING_SIZE> {
public:
    static auto GetThis() 
    {
        return decltype(this_)();
    }
    static void CleanupThis() {};

    static void Reset()
    {
        decltype(this_)::Reset();
    }

    static void *AllocBytes(size_t n_bytes)
    {
        return decltype(this_)::AllocBytes(n_bytes);
    }

    constexpr auto GetLastRegion()
    {
        return this_;
    }

private:
    Region<START_PTR, REMAINING_SIZE> this_;
};


template <uintptr_t START_PTR, size_t SIZE>
class GCRegion
{
public:
    template <typename T>
    class AllocatorRequirements
    {
    public:
        using value_type = T;
        AllocatorRequirements() = default;
        template <typename U>
        AllocatorRequirements(const AllocatorRequirements<U> &a2) {}
        [[nodiscard]] T* allocate(size_t n)
        {
            return GCRegion::Alloc<T>(n);
        }
        void deallocate(T *ptr, size_t n)
        {
            return;
        }
    };
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

    static void Reset()
    {
        decltype(survivors_)::Reset();
    }

    static void *AllocBytes(size_t n_bytes)
    {
        return decltype(survivors_)::AllocBytes(n_bytes);
    }
    void PrepareForSequentAllocations(size_t n_bytes);
    void EndSequentAllocations();

    static constexpr size_t TENURED_SIZE = SIZE / 4;
    static constexpr size_t SURVIVORS_N = 16;
    static constexpr size_t SURVIVORS_SIZE = (SIZE - TENURED_SIZE) / SURVIVORS_N;
    static constexpr size_t SURVIVORS_START_PTR = START_PTR;
    using RegionsPoolT = RegionsPool<SURVIVORS_START_PTR, SURVIVORS_SIZE, SURVIVORS_N, SIZE>;

private:
    RegionsPoolT survivors_;
};

}  // namespace k3s

#endif  // ALLOCATOR_GC_H
