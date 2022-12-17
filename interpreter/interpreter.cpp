#include "interpreter.h"
#include "generated/inst_decoder.h"
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

    k3s::Interpreter interp {};
    
    if (interp.LoadClassFile(file) != 0) {
        LOG_FATAL(INTERPRETER, "Loading failed");
    }

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

int Interpreter::LoadClassFile(FILE *fileptr) {
    ClassFileHeader header;
    int err_code = ClassFile::LoadClassFile(fileptr, &header, 
                                        &instructions_buffer_, &constant_pool_);
    if (err_code != 0) {
        return err_code;
    }
    SetPc(header.entry_point);
    SetProgram(instructions_buffer_.data());
    return 0;
}

int Interpreter::Invoke()
{
    auto &inst_buf = instructions_buffer_;
    InstDecoder decoder;
    auto *main_ptr = alloc_.AllocFunction(pc_);
    GetStateStack().emplace_back(-1, main_ptr);

#include "generated/dispatch_table.inl"

    // Begin execution:
    FETCH_AND_DISPATCH();

    LDAI:
    {
        const auto &elem = GetConstantPool().GetElement(decoder.GetImm());
        switch (elem.type_) {
            case Type::FUNC: {
                size_t bc_offs = GetConstantPool().GetFunctionBytecodeOffset(decoder.GetImm());
                auto *ptr = alloc_.AllocFunction(bc_offs);
                GetAcc().Set(Type::FUNC, bit_cast<uint64_t>(ptr));
                break;
            }
            case Type::NUM: {
                GetAcc().SetNum(bit_cast<double>(elem.val_)); 
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
    MOV_rANY_rANY:
    {
        GetReg(decoder.GetSecondReg()).Set(GetReg(decoder.GetFirstReg()));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    // Handlers:
    JUMP:
    {
        pc_ += decoder.GetImm();
        ASSERT(decoder.GetImm() != 0);
        FETCH_AND_DISPATCH();
    }
    
    BLE_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() <= 0.) {
            pc_ += decoder.GetImm();
            FETCH_AND_DISPATCH();
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }

    RET:
    {
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

    GETARG0_rANY: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(GetCallee()->GetArg<0>());
        ADVANCE_FETCH_AND_DISPATCH();
    }

    SETARG0_aFUNC_rANY: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        func_obj->SetArg<0>(GetReg(reg_id));
        ADVANCE_FETCH_AND_DISPATCH();
    }

    GETRET0_aFUNC_rANY: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(func_obj->GetRet<0>());
        ADVANCE_FETCH_AND_DISPATCH();
    }

    SETRET0_rANY: {
        size_t reg_id = decoder.GetFirstReg();
        GetCallee()->SetRet<0>(GetReg(reg_id));
        ADVANCE_FETCH_AND_DISPATCH();
    }


    CALL_aFUNC: {
        // return to the caller frame;
        // stack contains pc of the call instruction:
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue()); 
        GetStateStack().emplace_back(pc_, func_obj);
        pc_ = func_obj->GetTargetPc();
        FETCH_AND_DISPATCH();
    }

    ADD_rNUM_rNUM: {
        GetAcc().SetNum(GetReg(decoder.GetFirstReg()).GetAsNum() + GetReg(decoder.GetSecondReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB_rNUM_rNUM: {
        GetAcc().SetNum(GetReg(decoder.GetFirstReg()).GetAsNum() - GetReg(decoder.GetSecondReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV_rNUM_rNUM: {
        if (GetReg(decoder.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().SetNum(GetReg(decoder.GetFirstReg()).GetAsNum() / GetReg(decoder.GetSecondReg()).GetAsNum());
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL_rNUM_rNUM: {
        GetAcc().SetNum(GetReg(decoder.GetFirstReg()).GetAsNum() * GetReg(decoder.GetSecondReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    ADD_rSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aNUM_rNUM: {
        GetAcc().SetNum(GetAcc().GetAsNum() + GetReg(decoder.GetFirstReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aNUM_rNUM: {
        GetAcc().SetNum(GetAcc().GetAsNum() - GetReg(decoder.GetFirstReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aNUM_rNUM: {
        if (GetReg(decoder.GetFirstReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().SetNum(GetAcc().GetAsNum() / GetReg(decoder.GetFirstReg()).GetAsNum());
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aNUM_rNUM: {
        GetAcc().SetNum(GetAcc().GetAsNum() * GetReg(decoder.GetFirstReg()).GetAsNum());
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

    DECA_aNUM: {
        GetAcc().SetNum(GetAcc().GetAsNum() - 1.);
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
        GetReg(decoder.GetFirstReg()).Dump(decoder.GetFirstReg(), GetStateStack().size() - 1);
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DUMPA_aANY: {
        GetAcc().DumpAcc(GetStateStack().size() - 1);
        ADVANCE_FETCH_AND_DISPATCH();
    }
}

#undef FETCH_AND_DISPATCH

}  // namespace k3s 
