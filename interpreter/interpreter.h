#ifndef INTERPRETER_INTERPRETER_H
#define INTERPRETER_INTERPRETER_H

#include "bytecode_instruction.h"
#include "register.h"
#include <vector>
#include <array>

template <typename T>
using Vector = std::vector<T>;

namespace k3s {

class Interpreter {
public:
    
    // Returns after execution of Opcode::RET with empty call stack
    int Invoke(BytecodeInstruction *program, size_t pc = 0);

    void SetProgram(BytecodeInstruction *program) {
        program_ = program;
    }

    void SetPc(size_t pc) {
        state_.pc_ = pc;
    }

    const auto &Fetch() const
    {
        return program_[state_.pc_];
    }

    using Type = Register::Type;

    template <Type reg_type, Type... reg_types, typename... RegsIds>
    bool CheckRegsType(size_t reg_id, RegsIds... regs_ids) const
    {
        static_assert(sizeof...(reg_types) == sizeof...(RegsIds));

        const auto &reg = GetReg(reg_id);
        if constexpr (sizeof...(reg_types) != 0) {
            if (reg.GetType() == reg_type) {
                return CheckRegsType<reg_types...>(regs_ids...);
            }
        } else {
            return true;
        }
        return false;
    }

    template <Type acc_type, Type... reg_types, typename... RegsIds>
    bool CheckRegsTypeWithAcc(RegsIds... regs_ids) const
    {
        static_assert(sizeof...(reg_types) == (sizeof...(RegsIds)));

        auto &acc = GetAcc();
        if (acc.GetType() == acc_type) {
            return CheckRegsType<reg_types...>(regs_ids...);
        }
        return false;
    }

    const Register &GetReg(size_t id) const
    {
        return state_.regs_[id];
    }
    const Register &GetAcc() const
    {
        return state_.acc_;
    }
    auto &GetCallStack()
    {
        return state_.callstack_;
    }
private:
    struct InterpreterState {
    public:
        Register acc_ {};
        Register regs_[256] {};
        size_t pc_ {};
        Vector<size_t> callstack_ {};
    };

private:
    InterpreterState state_ {};
    BytecodeInstruction *program_ {};
};



}  // namespace k3s 

#endif  // INTERPRETER_INTERPRETER_H

