#ifndef INTERPRETER_TYPES_FUNCTION_H
#define INTERPRETER_TYPES_FUNCTION_H

#include "interpreter/register.h"
#include "allocator/allocator.h"
#include "allocator/object_header.h"
#include <cstddef>

namespace k3s::coretypes {

class Function : public ObjectHeader {
public:
    Function(size_t target_pc) : target_pc_(target_pc) {}

    size_t GetTargetPc() const {
        return target_pc_;
    }

    template <size_t i>
    Register *GetArg() {
        return &inputs_[i];
    }

    Register *GetArg(size_t i) {
        return &inputs_[i];
    }

    Register *GetThis() {
        return &this_;
    }

    template <size_t i>
    void SetArg(const Register &reg) {
        inputs_[i] = reg;
    }

    void SetThis(const Register &reg) {
        this_ = reg;
    }

    template <size_t i>
    Register *GetRet() {
        return &outputs_[i];
    }

    Register *GetRet(size_t i) {
        return &outputs_[i];
    }

    template <size_t i>
    void SetRet(const Register &reg) {
        outputs_[i] = reg;
    }

    template <uintptr_t START_PTR, size_t SIZE>
    static coretypes::Function *New(GCRegion<START_PTR, SIZE> reg, size_t bc_offs);

    static constexpr size_t INPUTS_COUNT = 4;
    static constexpr size_t OUTPUTS_COUNT = 4;
private:
    const size_t target_pc_ {};
    Register this_ {};
    Register inputs_[INPUTS_COUNT] {};
    Register outputs_[OUTPUTS_COUNT] {};
};

template <uintptr_t START_PTR, size_t SIZE>
inline coretypes::Function *Function::New(GCRegion<START_PTR, SIZE> reg, size_t bc_offs)
{
    size_t allocated_size = sizeof(coretypes::Function);
    void *storage = reg.AllocBytes(allocated_size);
    auto *ptr = new (storage) coretypes::Function(bc_offs);
    ptr->SetAllocatedSize(allocated_size);
    return ptr;
}

}

#endif  // INTERPRETER_TYPES_FUNCTION_H
