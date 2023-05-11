#ifndef INTERPRETER_TYPES_FUNCTION_H
#define INTERPRETER_TYPES_FUNCTION_H

#include "interpreter/register.h"
#include "allocator/allocator.h"
#include <cstddef>

namespace k3s::coretypes {

class Function {
public:
    Function(size_t target_pc) : target_pc_(target_pc) {}

    size_t GetTargetPc() const {
        return target_pc_;
    }

    template <size_t i>
    Register &GetArg() {
        return inputs_[i];
    }

    Register &GetThis() {
        return this_;
    }

    template <size_t i>
    void SetArg(const Register &reg) {
        inputs_[i] = reg;
    }

    void SetThis(const Register &reg) {
        this_ = reg;
    }

    template <size_t i>
    Register &GetRet() {
        return outputs_[i];
    }

    template <size_t i>
    void SetRet(const Register &reg) {
        outputs_[i] = reg;
    }

    template <uintptr_t START_PTR, size_t SIZE>
    static coretypes::Function *New(Allocator::Region<START_PTR, SIZE> reg, size_t bc_offs);

private:
    const size_t target_pc_ {};
    Register this_ {};
    Register inputs_[4U] {};
    Register outputs_[4U] {};
};

template <uintptr_t START_PTR, size_t SIZE>
inline coretypes::Function *Function::New(Allocator::Region<START_PTR, SIZE> reg, size_t bc_offs)
{
    void *ptr = reg.AllocBytes(sizeof(coretypes::Function));
    return new (ptr) coretypes::Function(bc_offs);
}

}

#endif  // INTERPRETER_TYPES_FUNCTION_H
