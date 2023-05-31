#include "interpreter.h"
#include "generated/inst_decoder.h"
#include "runtime/runtime.h"
#include "types/coretypes.h"
#include <cmath>

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
    pc_++;                                                          \
    auto &inst = Fetch();                                           \
    size_t dispatch_idx = decoder.DecodeAndResolve(inst, *this);    \
    ASSERT(DISPATCH_TABLE[dispatch_idx] != nullptr);                \
    goto *DISPATCH_TABLE[dispatch_idx];                             \
}

int Interpreter::Invoke()
{
    InstDecoder decoder;
    auto *main_ptr = coretypes::Function::New(Runtime::GetAllocator()->ObjectsRegion(), pc_);
    Runtime::GetInterpreter()->GetStateStack()->emplace_back(-1, main_ptr);

#include "generated/dispatch_table.inl"

    // Begin execution:
    FETCH_AND_DISPATCH();

    LDAI:
    {
        const auto &elem = Runtime::GetConstantPool()->GetElement(decoder.GetImm());
        switch (elem.type_) {
            case Type::FUNC: {
                size_t bc_offs = Runtime::GetConstantPool()->GetFunctionBytecodeOffset(decoder.GetImm());
                auto *ptr = coretypes::Function::New(Runtime::GetAllocator()->ObjectsRegion(), bc_offs);
                GetAcc().Set(ptr);
                break;
            } case Type::NUM: {
                GetAcc().Set(bit_cast<double>(elem.val_)); 
                break;
            } case Type::STR: {
                auto *ptr = coretypes::String::New(Runtime::GetAllocator()->ObjectsRegion(), reinterpret_cast<const char *>(elem.val_));
                GetAcc().Set(ptr); 
                break;
            } case Type::OBJ: {
                auto *ptr = coretypes::Object::New(Runtime::GetAllocator()->ObjectsRegion(), *Runtime::GetConstantPool()->GetMappingForObjAt(decoder.GetImm()), reinterpret_cast<size_t *>(elem.val_));
                GetAcc().Set(ptr); 
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

    STA_aANY:
    {
        GetReg(decoder.GetFirstReg()).Set(GetAcc());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    STNULL:
    {
        GetReg(decoder.GetFirstReg()).Set(Type::ANY, 0);
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MOV_rANY:
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
    BLT_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() < 0.) {
            pc_ += decoder.GetImm();
            FETCH_AND_DISPATCH();
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    BGE_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() >= 0.) {
            pc_ += decoder.GetImm();
            FETCH_AND_DISPATCH();
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    BNE_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() != 0.) {
            pc_ += decoder.GetImm();
            FETCH_AND_DISPATCH();
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }

    RET:
    {
        if (Runtime::GetInterpreter()->GetStateStack()->size() == 1) {
            return decoder.GetImm();
        } else {
            // return to the caller frame;
            // stack contains pc of the call instruction:
            pc_ = Runtime::GetInterpreter()->GetStateStack()->back().caller_pc_;
            pc_++;
            Runtime::GetInterpreter()->GetStateStack()->pop_back();
        }
        FETCH_AND_DISPATCH();
    }

    GETARG0: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetArg<0>());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETARG1: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetArg<1>());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETTHIS: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetThis());
        ADVANCE_FETCH_AND_DISPATCH();
    }

    SETARG0_aFUNC_rANY: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        func_obj->SetArg<0>(GetReg(reg_id));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETARG1_aFUNC_rANY: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        func_obj->SetArg<1>(GetReg(reg_id));
        ADVANCE_FETCH_AND_DISPATCH();
    }

    GETRET0_aFUNC: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*func_obj->GetRet<0>());
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
        auto *func_obj = GetAcc().GetAsFunction(); 
        Runtime::GetInterpreter()->GetStateStack()->emplace_back(pc_, func_obj);
        pc_ = func_obj->GetTargetPc();
        FETCH_AND_DISPATCH();
    }

    ADD_rNUM_rNUM: {
        GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() + GetReg(decoder.GetSecondReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB_rNUM_rNUM: {
        GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() - GetReg(decoder.GetSecondReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV_rNUM_rNUM: {
        if (GetReg(decoder.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() / GetReg(decoder.GetSecondReg()).GetAsNum());
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MOD_rNUM_rNUM: {
        if (GetReg(decoder.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(std::fmod(GetReg(decoder.GetFirstReg()).GetAsNum(), GetReg(decoder.GetSecondReg()).GetAsNum()));
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL_rNUM_rNUM: {
        GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() * GetReg(decoder.GetSecondReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    ADD_rSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aNUM_rNUM: {
        GetAcc().Set(GetAcc().GetAsNum() + GetReg(decoder.GetFirstReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aNUM_rNUM: {
        GetAcc().Set(GetAcc().GetAsNum() - GetReg(decoder.GetFirstReg()).GetAsNum());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aNUM_rNUM: {
        if (GetReg(decoder.GetFirstReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(GetAcc().GetAsNum() / GetReg(decoder.GetFirstReg()).GetAsNum());
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aNUM_rNUM: {
        GetAcc().Set(GetAcc().GetAsNum() * GetReg(decoder.GetFirstReg()).GetAsNum());
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
        GetAcc().Set(GetAcc().GetAsNum() - 1.);
        ADVANCE_FETCH_AND_DISPATCH();
    }

    NEWARR_rNUM: {
        size_t reg_id = decoder.GetFirstReg();
        size_t arr_sz = static_cast<size_t>(GetReg(reg_id).GetAsNum());
        GetAcc().Set(coretypes::Array::New(Runtime::GetAllocator()->ObjectsRegion(), arr_sz));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETELEM_aANY_rARR_rNUM: {
        size_t idx = static_cast<size_t>(GetReg(decoder.GetSecondReg()).GetAsNum());
        GetReg(decoder.GetFirstReg()).GetAsArray()->SetElem(idx, GetAcc());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETELEM_aANY_rOBJ_rSTR: {
        auto string = GetReg(decoder.GetSecondReg()).GetAsString();
        GetReg(decoder.GetFirstReg()).GetAsObject()->SetElem(string->GetData(), GetAcc());
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETELEM_rARR_rNUM: {
        size_t idx = static_cast<size_t>(GetReg(decoder.GetSecondReg()).GetAsNum());
        GetAcc().Set(*GetReg(decoder.GetFirstReg()).GetAsArray()->GetElem(idx));
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETELEM_rOBJ_rSTR: {
        auto string = GetReg(decoder.GetSecondReg()).GetAsString();
        GetAcc().Set(*GetReg(decoder.GetFirstReg()).GetAsObject()->GetElem(string->GetData()));
        if (GetAcc().GetType() == Type::FUNC) {
            GetAcc().GetAsFunction()->SetThis(GetReg(decoder.GetFirstReg()));
        }
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DUMP_rANY: {
        GetReg(decoder.GetFirstReg()).Dump();
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DUMPA_aANY: {
        GetAcc().Dump();
        ADVANCE_FETCH_AND_DISPATCH();
    }
}

#undef FETCH_AND_DISPATCH

}  // namespace k3s 
