#ifndef ALLOCATOR_GC_H
#define ALLOCATOR_GC_H

#include "allocator/containers.h"
#include "allocator/object_header.h"
#include <chrono>


namespace k3s {

class GC
{
public:
    GC()
    {
        timestamp_ = std::chrono::steady_clock::now();
    }
    struct DanglingReference
    {
    public:
        DanglingReference(ObjectHeader *ref_holder_obj, ptrdiff_t offset) : ref_holder(ref_holder_obj),
                                                                            offset_to_dangling_ref(offset)
        {
            ASSERT(offset != 0);
        }

        DanglingReference(ObjectHeader **ref_ptr) : ref_ptr(ref_ptr),
                                                    offset_to_dangling_ref(0) {}

        DanglingReference() : ref_ptr(nullptr),
                              offset_to_dangling_ref(0) {}

        ObjectHeader **GetDanglingRefFrom(ObjectHeader *base_ptr) const
        {
            auto *ptr = reinterpret_cast<char *>(base_ptr) + offset_to_dangling_ref;
            return reinterpret_cast<ObjectHeader **>(ptr);
        }
        
        ObjectHeader **GetDanglingRef() const
        {
            return GetDanglingRefFrom(GetRefHolder());
        }

        bool IsEmpty() const
        {
            if (offset_to_dangling_ref == 0) {
                return GetRefPtr() == nullptr;
            }
            ASSERT(GetRefHolder() != nullptr);
            return false;
        }

        void Fix() const
        {
            if (offset_to_dangling_ref == 0) {
                auto new_ref = (*GetRefPtr())->GetRelocatedPtr();
                ASSERT(new_ref != nullptr);
                *GetRefPtr() = new_ref;
                return;
            }
            
            // For both cases below `new_ref` should be valid
            auto new_ref = (*GetDanglingRef())->GetRelocatedPtr();
            ASSERT(new_ref != nullptr);
            if (GetRefHolder()->WasRelocated()) {
                // case 1: `ref_holder` was relocated (`dangling_ref_holder` was in region being cleaned):
                ASSERT((*GetDanglingRefFrom(GetRefHolder()->GetRelocatedPtr()))->GetRelocatedPtr() == new_ref);
                *GetDanglingRefFrom(GetRefHolder()->GetRelocatedPtr()) = new_ref;
            } else {
                // case 2: `ref_holder` wasn't relocated (`dangling_ref_holder` wasn't in region being cleaned):
                *GetDanglingRef() = new_ref;
            }
        }

        ObjectHeader *GetRefHolder() const
        {
            ASSERT(offset_to_dangling_ref != 0);
            return ref_holder;
        }
        ObjectHeader **GetRefPtr() const
        {
            ASSERT(offset_to_dangling_ref == 0);
            return ref_ptr;
        }
    public:
        // if (offset_to_dangling_ref == 0) -> dangling_ref_holder should be casted to ObjectHeader **
        // if (offset_to_dangling_ref != 0) -> dangling_ref_holder should be casted to ObjectHeader *
        union {
            ObjectHeader *ref_holder;
            ObjectHeader **ref_ptr;
        };
        ptrdiff_t offset_to_dangling_ref;
    };
    
    struct GCStageState
    {
    public:
        GCVector<ObjectHeader *> target_objects;
        size_t estimating_realloc_size;
        GCVector<DanglingReference> references_to_targets;
    };

public:
    void PrepareNewStage()
    {
        if (stages_stack_.size() == 0) {
            auto newstamp = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(newstamp - timestamp_).count();
            LOG_DEBUG(GC, "Code executed for = " << diff << "[us]\n");
            timestamp_ = newstamp;
        }
        stages_stack_.emplace_back();
    }

    void FinalizeStage();

    void AppendAliveObject(ObjectHeader *obj)
    {
        ASSERT(obj != nullptr);
        stages_stack_.back().estimating_realloc_size += obj->GetAllocatedSize();
        stages_stack_.back().target_objects.push_back(obj);
    }

    void AppendRefToAliveObject(ObjectHeader *ref_holder, ObjectHeader **ref)
    {
        ptrdiff_t bytes_offset = reinterpret_cast<char *>(ref)  - reinterpret_cast<char *>(ref_holder);
        ASSERT(bytes_offset >= 0);
        stages_stack_.back().references_to_targets.emplace_back(ref_holder, bytes_offset);
    }
    void AppendRefToAliveObject(ObjectHeader **ref_ptr)
    {
        stages_stack_.back().references_to_targets.emplace_back(ref_ptr);
    }
    size_t GetEstimatedSpace()
    {
        return stages_stack_.back().estimating_realloc_size;
    }

    ObjectHeader *PopAliveObject()
    {
        if (stages_stack_.back().target_objects.size() == 0) {
            return nullptr;
        }
        auto *obj = stages_stack_.back().target_objects.back();
        stages_stack_.back().target_objects.pop_back();
        ASSERT(obj != nullptr);
        ASSERT(!obj->WasRelocated());
        return obj;
    }
    DanglingReference PopRefToMovedObject()
    {
        if (stages_stack_.back().references_to_targets.size() == 0) {
            return {};
        }
        auto ref = stages_stack_.back().references_to_targets.back();
        stages_stack_.back().references_to_targets.pop_back();
        return ref;
    }
    // This should be called before each `MoveObjects` phase
    void FixInvalidDanglingReferences()
    {
        for (auto &stage : stages_stack_) {
            for (auto &dangling_ref : stage.references_to_targets) {
                // if (dangling_ref.offset_to_dangling_ref == 0) then ref is stored in root (vreg)
                if ((dangling_ref.offset_to_dangling_ref != 0) && (dangling_ref.GetRefHolder()->WasRelocated())) {
                    dangling_ref.ref_holder = dangling_ref.GetRefHolder()->GetRelocatedPtr();
                }
            }
        }
    }

    void InitMark()
    {
        mark_ += 1; 
    }

    auto GetMark()
    {
        return mark_; 
    }
    void ForbidTrigger()
    {
        is_trigger_forbidden_ = true;
    }
    void AllowTrigger()
    {
        is_trigger_forbidden_ = false;
    }
    bool IsTriggerForbidden()
    {
        return is_trigger_forbidden_;
    }

private:
    std::chrono::steady_clock::time_point timestamp_;
    bool is_trigger_forbidden_ {false};
    ObjectHeader::MarkT mark_ {0};
    GCVector<GCStageState> stages_stack_;
};

}  // namespace k3s

#endif  // ALLOCATOR_GC_H
