#include "runtime/runtime.h"

namespace k3s {
#define REGIONS_POOL_ARGS() template <uintptr_t START_PTR, size_t REGION_SIZE, size_t N_REGIONS, size_t REMAINING_SIZE>
#define REGIONS_POOL() RegionsPool<START_PTR, REGION_SIZE, N_REGIONS, REMAINING_SIZE>

#define GC_REGION_ARGS() template <uintptr_t START_PTR, size_t SIZE>
#define GC_REGION() GCRegion<START_PTR, SIZE>

    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::CleanupThis()
    {
        if (Runtime::GetGC()->IsTriggerForbidden()) {
            LOG_FATAL(GC, "GC Trigger was forbidden");
        }
        LOG_DEBUG(GC, "Cleanup region " << N_REGIONS << " (start_ptr = " << START_PTR << ")");
        Runtime::GetGC()->PrepareNewStage();
        MarkAndFetchTargetObjects();
        MoveObjects();
        RebindLinks();
        decltype(this_)::Reset();
        Runtime::GetGC()->FinalizeStage();
    }
   
    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::MarkAndFetchTargetObjects()
    {
        auto &state_stack = *Runtime::GetInterpreter()->GetStateStack();
        Runtime::GetGC()->InitMark();
        
        for (auto &state : state_stack) {
            if (MarkAndFetchRecursively(Register(state.callee_))) {
                Runtime::GetGC()->AppendRefToAliveObject(reinterpret_cast<ObjectHeader **>(&state.callee_));
            }
            if (MarkAndFetchRecursively(state.acc_)) {
                Runtime::GetGC()->AppendRefToAliveObject(state.acc_.GetObjectHeaderPtr());
            }
            for (auto &vreg : state.regs_) {
                if (MarkAndFetchRecursively(vreg)) {
                    Runtime::GetGC()->AppendRefToAliveObject(vreg.GetObjectHeaderPtr());
                }
            }
        }
    }

    REGIONS_POOL_ARGS()
    bool REGIONS_POOL()::MarkAndFetchRecursively(Register vreg)
    {
        if (vreg.IsPrimitive()) {
            return false;
        }
        auto *obj_header = vreg.GetAsObjectHeader(); 
        bool should_be_realocated = GetThis().Contains(obj_header);
        if (vreg.GetAsObjectHeader()->IsMarked(Runtime::GetGC()->GetMark())) {
            return should_be_realocated;
        }
        obj_header->Mark(Runtime::GetGC()->GetMark());

        if (should_be_realocated) {
            Runtime::GetGC()->AppendAliveObject(obj_header);
        }
    
        switch (vreg.GetType())
        {
        case Register::Type::ARR:
            MarkArray(obj_header);
            break;
        case Register::Type::OBJ:
            MarkObject(obj_header);
            break;
        case Register::Type::FUNC:
            MarkFunction(obj_header);
            break;
        case Register::Type::STR:
            break;
        default:
            LOG_FATAL(GC, "Unexpected object type");
        }
        return should_be_realocated;
    }

    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::MarkArray(ObjectHeader *obj)
    {
        auto *array = static_cast<coretypes::Array *>(obj);
        for (size_t i = 0; i < array->GetSize(); i++) {
            auto *array_elem = array->GetElem(i);
            if (MarkAndFetchRecursively(*array_elem)) {
                Runtime::GetGC()->AppendRefToAliveObject(obj, array_elem->GetObjectHeaderPtr());
            }
        }
    }

    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::MarkObject(ObjectHeader *obj)
    {
        auto *object = static_cast<coretypes::Object *>(obj);
        for (size_t i = 0; i < object->GetSize(); i++) {
            auto *obj_field = object->GetElem(i);
            if (MarkAndFetchRecursively(*obj_field)) {
                Runtime::GetGC()->AppendRefToAliveObject(obj, obj_field->GetObjectHeaderPtr());
            }
        }
    }

    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::MarkFunction(ObjectHeader *obj)
    {
        auto *func = static_cast<coretypes::Function *>(obj);

        auto *reg = func->GetThis();
        if (MarkAndFetchRecursively(*reg)) {
            Runtime::GetGC()->AppendRefToAliveObject(obj, reg->GetObjectHeaderPtr());
        }
        for (size_t i = 0; i < coretypes::Function::INPUTS_COUNT; i++) {
            auto input_reg = func->GetArg(i);
            if (MarkAndFetchRecursively(*input_reg)) {
                Runtime::GetGC()->AppendRefToAliveObject(obj, input_reg->GetObjectHeaderPtr());
            }
        }
        for (size_t i = 0; i < coretypes::Function::OUTPUTS_COUNT; i++) {
            auto output_reg = func->GetRet(i);
            if (MarkAndFetchRecursively(*output_reg)) {
                Runtime::GetGC()->AppendRefToAliveObject(obj, output_reg->GetObjectHeaderPtr());
            }
        }
    }
    
    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::MoveObjects()
    {
        if (GetOthers().GetThis().GetRemainingSpace() < Runtime::GetGC()->GetEstimatedSpace()) {
            GetOthers().CleanupThis();
            ASSERT(GetOthers().GetThis().GetRemainingSpace() >= Runtime::GetGC()->GetEstimatedSpace());
            Runtime::GetGC()->FixInvalidDanglingReferences();
        }
        auto *obj = Runtime::GetGC()->PopAliveObject();
        while (obj != nullptr) {
            size_t obj_size = obj->GetAllocatedSize();
            void *new_ptr = GetOthers().GetThis().AllocBytes(obj_size);
            std::memcpy(new_ptr, obj, obj_size);
            obj->SetRelocatedPtr(new_ptr);
            obj = Runtime::GetGC()->PopAliveObject();
        }
    }

    REGIONS_POOL_ARGS()
    void REGIONS_POOL()::RebindLinks()
    {
        auto dangling_ref = Runtime::GetGC()->PopRefToMovedObject();
        while (!dangling_ref.IsEmpty()) {
            dangling_ref.Fix();
            dangling_ref = Runtime::GetGC()->PopRefToMovedObject();
        }
    }
    
    GC_REGION_ARGS()
    void GC_REGION()::PrepareForSequentAllocations(size_t n_bytes)
    {
        if (survivors_.GetThis().GetRemainingSpace() <= n_bytes) {
            survivors_.CleanupThis();
            ASSERT(survivors_.GetThis().GetRemainingSpace() > n_bytes);
        }
        Runtime::GetGC()->ForbidTrigger();
    }
    GC_REGION_ARGS()
    void GC_REGION()::EndSequentAllocations()
    {
        Runtime::GetGC()->AllowTrigger();
    }

    void GC::FinalizeStage()
    {
        ASSERT(stages_stack_.size() > 0);
        stages_stack_.pop_back();

        if (stages_stack_.size() == 0) {
            Runtime::GetAllocator()->GcInternalsRegion().Reset();
            decltype(stages_stack_) tmp;
            ASSERT(tmp.capacity() == 0);
            ASSERT(tmp.data() == nullptr);
            stages_stack_.swap(tmp);
            
            auto newstamp = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(newstamp - timestamp_).count();
            std::cout << "Spent in GC = " << diff << "[us]\n";
            timestamp_ = newstamp;
        }
    }

template class GCRegion<251658240ul, 16777216ul>;
// For some reasons, in release builds `GCRegion` instantiation is not enough:
template class RegionsPool<251658240ul, 786432ul, 16ul, 16777216ul>;

}  // namespace k3s
