#ifndef INTERPRETER_TYPES_FUNCTION_H
#define INTERPRETER_TYPES_FUNCTION_H

#include "interpreter/register.h"
#include <cstddef>

namespace k3s::coretypes {

class Function {
public:
    Function(size_t target_pc) : target_pc_(target_pc) {}
    Function(const Function &f) = default;

    size_t GetTargetPc() const {
        return target_pc_;
    }
private:
    const size_t target_pc_ {};
    Register inputs_[4U] {};
    Register outputs_[4U] {};
};

}

#endif  // INTERPRETER_TYPES_FUNCTION_H
