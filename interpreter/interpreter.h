#ifndef INTERPRETER_INTERPRETER_H
#define INTERPRETER_INTERPRETER_H

#include "bytecode_instruction.h"
#include "register.h"
#include "allocator/containers.h"
#include "classfile/class_file.h"
#include <iomanip>
#include <fstream>

namespace k3s {

class Interpreter {
public:
    Interpreter()
    {
        state_stack_.reserve(Allocator::StackRegionT::MAX_ALLOC_SIZE / sizeof(InterpreterState));
    }

    // Returns after execution of Opcode::RET with empty call stack
    int Invoke();

    void SetProgram(BytecodeInstruction *program)
    {
        program_ = program;
    }

    void SetPc(size_t pc)
    {
        pc_ = pc;
    }

    size_t GetPc()
    {
        return pc_;
    }

    const auto &Fetch() const
    {
        // TODO: if constexpr
        #if K3S_FETCH_HOOK
        FetchHook(hook_data_);
        #endif
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
    auto *GetStateStack()
    {
        return &state_stack_;
    }

    void DumpTrace(const char* cmd_name)
    {
        auto state = state_stack_.back();
        trace_file_ << cmd_name << std::endl;
        trace_file_ << "pc: " << pc_ << std::endl; 
        for (int i = 0; i < InterpreterState::REGS_NUM / 4; ++i) {
            trace_file_ << std::left << "r" << i << ": " <<  std::setw(20) << state.regs_[i].GetValue()
                        << "r" << i + 4 << ": " << std::setw(20) << state.regs_[i+4].GetValue()
                        << "r" << i + 8 << ": " << std::setw(20) << state.regs_[i+8].GetValue()
                        << "r" << i + 12 << ": " << std::setw(20) << state.regs_[i+12].GetValue() << std::endl;
        }
        trace_file_ << "\n";

    }

private:
    struct InterpreterState {
        static constexpr size_t REGS_NUM = 16U;

        InterpreterState(size_t caller_pc, coretypes::Function *callee_obj)
        {
            caller_pc_ = caller_pc;
            callee_ = callee_obj;
        }
    public:
        Register acc_ {};
        Register regs_[REGS_NUM];
        size_t caller_pc_ {};
        // This is used for implicit this inside functions:
        coretypes::Function *callee_ = nullptr;
    };

private:
    size_t pc_ {};
    StackVector<InterpreterState> state_stack_;
    BytecodeInstruction *program_ {};
    std::ofstream trace_file_;
#ifdef K3S_FETCH_HOOK
    void *hook_data_ = nullptr;
    void FetchHook(void *hook_data) const;
public:
    void SetHookData(void *data) { hook_data_ = data; }
#endif
};

}  // namespace k3s 

#endif  // INTERPRETER_INTERPRETER_H

