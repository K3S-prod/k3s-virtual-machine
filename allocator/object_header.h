#ifndef ALLOCATOR_OBJECT_HEADER_H
#define ALLOCATOR_OBJECT_HEADER_H

#include <cstddef>
#include <cstdint>
#include <common/macro.h>

namespace k3s {

class ObjectHeader
{
public:
    using MarkT = uint64_t;

#ifndef NDEBUG
#define CHECK() if (_debug_mark_ != 0xCAFE) { LOG_FATAL(ObjectHeader, "Accessed invalid object header"); } 
#else
#define CHECK()
#endif

    void Mark(MarkT mark)
    {
        CHECK();
        mark_ = mark;
    }
    
    bool IsMarked(MarkT mark)
    {
        CHECK();
        return mark_ == mark;
    }

    void SetAllocatedSize(size_t size)
    {
        CHECK();
        allocated_size_ = size;
    }
    auto GetAllocatedSize() const
    {
        CHECK();
        return allocated_size_;
    }
    void SetRelocatedPtr(void *ptr)
    {
        CHECK();
        // An Object should be relocated less than once per gc trigger:
        ASSERT(relocated_ptr_ == nullptr);
        relocated_ptr_ = reinterpret_cast<ObjectHeader *>(ptr);
    }
    auto GetRelocatedPtr() const
    {
        CHECK();
        return relocated_ptr_;
    }
    bool WasRelocated() const
    {
        CHECK();
        return relocated_ptr_ != nullptr;
    }

private:
#ifndef NDEBUG
    uint16_t _debug_mark_ {0xCAFE};
#endif
    MarkT mark_ {};
    size_t allocated_size_ {};
    ObjectHeader *relocated_ptr_ {};
};

}

#endif
