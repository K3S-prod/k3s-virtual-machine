#include "interpreter.h"
#include "generated/inst_decoder.h"
#include "assembler/assembler.h"
#include "types/coretypes.h"

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

    k3s::Interpreter interp {};
    interp.Invoke();

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
    pc_++;                                                   \
    auto &inst = Fetch();                                           \
    size_t dispatch_idx = decoder.DecodeAndResolve(inst, *this);    \
    ASSERT(DISPATCH_TABLE[dispatch_idx] != nullptr);                \
    goto *DISPATCH_TABLE[dispatch_idx];                             \
}


int Interpreter::Invoke()
{
    auto main_id = AsmEncoder::ResolveName("main");
    auto main_pc = AsmEncoder::GetConstantPool().GetFunctionBytecodeOffset(main_id);
    auto &inst_buf = k3s::AsmEncoder::GetInstructionsBuffer();
    SetProgram(inst_buf.data());
    SetPc(main_pc);
    InstDecoder decoder;
    GetStateStack().push_back(-1);

#include "generated/dispatch_table.inl"

    // Begin execution:
    FETCH_AND_DISPATCH();

    LDAI:
    {
        const auto &elem = AsmEncoder::GetConstantPool().GetElement(decoder.GetImm());
        switch (elem.type_) {
            case Type::FUNC: {
                void *storage = alloc_.AllocBytes(sizeof(coretypes::Function));
                size_t bc_offs = AsmEncoder::GetConstantPool().GetFunctionBytecodeOffset(decoder.GetImm());
                new (storage) coretypes::Function(bc_offs);
                GetAcc().Set(Type::FUNC, bit_cast<uint64_t>(storage));
                break;
            }
            case Type::NUM: {
                GetAcc().Set(Type::NUM, elem.val_); 
                break;
            }
            default: {
                LOG_FATAL(INTERPRETER, "Unknown constant pool element type for index (i = " << static_cast<int64_t>(decoder.GetImm()) << ")" );
            }
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }

    LDA_rANY:
    {
        GetAcc().Set(GetReg(decoder.GetFirstReg()));
        ADVANCE_FETCH_AND_DISPATCH();

    }

    STA_rANY:
    {
        GetReg(decoder.GetFirstReg()).Set(GetAcc());
        ADVANCE_FETCH_AND_DISPATCH();
    }

    // Handlers:
    JUMP:
    {
        printf("JUMP\n");

        pc_ += decoder.GetImm();
        ASSERT(decoder.GetImm() != 0);
        FETCH_AND_DISPATCH();
    }

    RET:
    {
        printf("RET\n");
        if (GetStateStack().size() == 1) {
            return decoder.GetImm();
        } else {
            // return to the caller frame;
            // stack contains pc of the call instruction:
            pc_ = GetStateStack().back().caller_pc_;
            pc_++;
            GetStateStack().pop_back();
        }
        FETCH_AND_DISPATCH();
    }

    CALL_aFUNC: {
        printf("CALL_aFUNC\n");
        // return to the caller frame;
        // stack contains pc of the call instruction:
        const auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue()); 
        pc_ = func_obj->GetTargetPc();
        GetStateStack().push_back(pc_);
        FETCH_AND_DISPATCH();
    }

    ADD_rNUM_rNUM: {
        GetAcc().Set(Type::NUM, (GetReg(decoder.GetFirstReg()).GetAsNum() + GetReg(decoder.GetSecondReg()).GetAsNum()));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB_rNUM_rNUM: {
        GetAcc().Set(Type::NUM, (GetReg(decoder.GetFirstReg()).GetAsNum() - GetReg(decoder.GetSecondReg()).GetAsNum()));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV_rNUM_rNUM: {
        if (GetReg(decoder.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(Type::NUM, (GetReg(decoder.GetFirstReg()).GetAsNum() / GetReg(decoder.GetSecondReg()).GetAsNum()));
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL_rNUM_rNUM: {
        GetAcc().Set(Type::NUM, (GetReg(decoder.GetFirstReg()).GetAsNum() * GetReg(decoder.GetSecondReg()).GetAsNum()));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    ADD_rSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aNUM_rNUM: {
        GetAcc().Set(Type::NUM, (GetAcc().GetAsNum() + GetReg(decoder.GetFirstReg()).GetAsNum()));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aNUM_rNUM: {
        GetAcc().Set(Type::NUM, (GetAcc().GetAsNum() - GetReg(decoder.GetFirstReg()).GetAsNum()));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aNUM_rNUM: {
        if (GetReg(decoder.GetFirstReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(Type::NUM, (GetAcc().GetAsNum() / GetReg(decoder.GetFirstReg()).GetAsNum()));
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aNUM_rNUM: {
        GetAcc().Set(Type::NUM, (GetAcc().GetAsNum() * GetReg(decoder.GetFirstReg()).GetAsNum()));
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
    DUMP_rANY: {
        GetReg(decoder.GetFirstReg()).Dump();
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DUMP_aANY: {
        GetAcc().Dump();
        ADVANCE_FETCH_AND_DISPATCH();
    }
}

#undef FETCH_AND_DISPATCH

}  // namespace k3s 
