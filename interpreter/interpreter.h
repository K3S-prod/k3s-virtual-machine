#ifndef INTERPRETER_INTERPRETER_H
#define INTERPRETER_INTERPRETER_H

#include "bytecode_instruction.h"
#include "register.h"
#include "allocator/containers.h"
#include "classfile/class_file.h"

namespace k3s {

class Interpreter {
public:
    // Returns after execution of Opcode::RET with empty call stack
    int Invoke();

    int LoadClassFile(const char *fn);

    void SetProgram(BytecodeInstruction *program) {
        program_ = program;
    }

    void SetPc(size_t pc) {
        pc_ = pc;
    }

    const auto &Fetch() const
    {
        return program_[pc_];
    }

    using Type = Register::Type;

    template <Type reg_type, Type... reg_types, typename... RegsIds>
    bool CheckRegsType(size_t reg_id, RegsIds... regs_ids) const
    {
        static_assert(sizeof...(reg_types) == sizeof...(RegsIds));

        const auto &reg = GetReg(reg_id);
        if constexpr (sizeof...(reg_types) != 0) {
            if ((reg_type == Type::ANY) || (reg.GetType() == reg_type)){
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
        if ((acc_type == Type::ANY) || (acc.GetType() == acc_type)) {
            if constexpr (sizeof...(reg_types) != 0) {
                return CheckRegsType<reg_types...>(regs_ids...);
            }
            return true;
        }
        return false;
    }

    const Register &GetReg(size_t id) const
    {
        return state_stack_.back().regs_[id];
    }
    Register &GetReg(size_t id)
    {
        return state_stack_.back().regs_[id];
    }

    const Register &GetAcc() const
    {
        return state_stack_.back().acc_;
    }
    Register &GetAcc()
    {
        return state_stack_.back().acc_;
    }
    coretypes::Function *GetCallee()
    {
        return state_stack_.back().callee_;
    }
    auto &GetStateStack()
    {
        return state_stack_;
    }
    auto &GetConstantPool()
    {
        return constant_pool_;
    }
private:
    struct InterpreterState {
    public:
        InterpreterState(size_t caller_pc, coretypes::Function *callee_obj)
        {
            caller_pc_ = caller_pc;
            callee_ = callee_obj;
            regs_.resize(16);
        }
    public:
        Register acc_ {};
        ArenaVector<Register> regs_;
        size_t caller_pc_ {};
        // This is used for implicit this inside functions:
        coretypes::Function *callee_ = nullptr;
    };

private:
    Allocator allocator_ {};
    size_t pc_ {};
    ArenaVector<InterpreterState> state_stack_;
    BytecodeInstruction *instructions_buffer_{};
    BytecodeInstruction *program_ {};
    ConstantPool constant_pool_{};
};

}  // namespace k3s 

#endif  // INTERPRETER_INTERPRETER_H

