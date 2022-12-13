#include "interpreter.h"
#include "generated/inst_decoder.h"
#include "assembler/assembler.h"


int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 1;
    }
    auto file = std::fopen(argv[1], "r");
    if (file == nullptr) {
        return 2;
    }

    if (k3s::AsmEncoder::Process(file) != 0) {
        LOG_FATAL(INTERPRETER, "Parsing failed");
    }
    auto inst_buf = k3s::AsmEncoder::GetInstructionsBuffer();

    k3s::Interpreter interp{};
    interp.Invoke(inst_buf.data());

    return 0;
}

namespace k3s {

std::ostream &operator<<(std::ostream &os, const BytecodeInstruction &inst)
{
    return *inst.Dump(&os);
}

#define FETCH_AND_DISPATCH() \
{                                                                   \
    auto &inst = Fetch();                                           \
    size_t dispatch_idx = decoder.DecodeAndResolve(inst, *this);    \
    ASSERT(DISPATCH_TABLE[dispatch_idx] != nullptr);                \
    goto *DISPATCH_TABLE[dispatch_idx];                             \
}

#define ADVANCE_FETCH_AND_DISPATCH() \
{                                                                   \
    state_.pc_++;                                                   \
    auto &inst = Fetch();                                           \
    size_t dispatch_idx = decoder.DecodeAndResolve(inst, *this);    \
    ASSERT(DISPATCH_TABLE[dispatch_idx] != nullptr);                \
    goto *DISPATCH_TABLE[dispatch_idx];                             \
}


int Interpreter::Invoke(BytecodeInstruction *program, size_t pc)
{
    SetProgram(program);
    SetPc(pc);
#include "generated/dispatch_table.inl"
    InstDecoder decoder;

    FETCH_AND_DISPATCH();

    // Handlers:
    JUMP:
    {
        printf("JUMP\n");

        state_.pc_ += decoder.GetImm();
        ASSERT(decoder.GetImm() != 0);
        FETCH_AND_DISPATCH();
    }

    RET:
    {
        printf("RET\n");
        if (GetCallStack().empty()) {
            return decoder.GetImm();
        } else {
            // return to the caller frame;
            // stack contains pc of the call instruction:
            state_.pc_ = GetCallStack().back();
            state_.pc_++;
            GetCallStack().pop_back();
        }
        FETCH_AND_DISPATCH();
    }

    ADD_rNUM_rNUM: {
        StoreToAcc((*this).GetReg(decoder.GetFirstReg()).GetValue() + (*this).GetReg(decoder.GetSecondReg()).GetValue());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB_rNUM_rNUM: {
        StoreToAcc((*this).GetReg(decoder.GetFirstReg()).GetValue() - (*this).GetReg(decoder.GetSecondReg()).GetValue());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV_rNUM_rNUM: {
        auto second_reg_value = (*this).GetReg(decoder.GetSecondReg()).GetValue();
        if (second_reg_value == 0)
        {
            throw std::runtime_error("Division by zero detected!");
        } else {
            StoreToAcc((*this).GetReg(decoder.GetFirstReg()).GetValue() / second_reg_value);
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL_rNUM_rNUM: {
        StoreToAcc((*this).GetReg(decoder.GetFirstReg()).GetValue() * (*this).GetReg(decoder.GetSecondReg()).GetValue());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    ADD_rSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aNUM_rNUM: {
        StoreToAcc((*this).GetAcc().GetValue() +  (*this).GetReg(decoder.GetSecondReg()).GetValue());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aNUM_rNUM: {
        StoreToAcc((*this).GetAcc().GetValue() -  (*this).GetReg(decoder.GetSecondReg()).GetValue());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aNUM_rNUM: {
        auto second_reg_value = (*this).GetReg(decoder.GetSecondReg()).GetValue();
        if (second_reg_value == 0)
        {
            throw std::runtime_error("Division by zero detected!");
        } else {
            StoreToAcc((*this).GetAcc().GetValue() / second_reg_value);
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aNUM_rNUM: {
        StoreToAcc((*this).GetAcc().GetValue() *  (*this).GetReg(decoder.GetSecondReg()).GetValue());
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    NEWARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETELEM_aARR_rNUM_rANY: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETELEM_aARR_rNUM_rANY: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

}


#undef FETCH_AND_DISPATCH

}  // namespace k3s 
