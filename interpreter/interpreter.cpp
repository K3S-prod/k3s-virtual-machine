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

#define FETCH_AND_DISPATCH()                                        \
{                                                                   \
    auto &inst = Fetch();                                           \
    size_t dispatch_idx = decoder.DecodeAndResolve(inst, *this);    \
    ASSERT(DISPATCH_TABLE[dispatch_idx] != nullptr);                \
    goto *DISPATCH_TABLE[dispatch_idx];                             \
}

#define ADVANCE_FETCH_AND_DISPATCH()                                \
{                                                                   \
    pc_++;                                                          \
    auto &inst = Fetch();                                           \
    size_t dispatch_idx = decoder.DecodeAndResolve(inst, *this);    \
    ASSERT(DISPATCH_TABLE[dispatch_idx] != nullptr);                \
    goto *DISPATCH_TABLE[dispatch_idx];                             \
}

#ifdef TRACE
#define DUMP_TRACE(cmd) DumpTrace(cmd)
#else
#define DUMP_TRACE(cmd)
#endif // TRACE

int Interpreter::Invoke()
{
    #ifdef TRACE
    trace_file_.open("trace.log");
    if (!trace_file_.is_open()) {
        LOG_FATAL(ASSEMBLER, "Can't open trace file");
    }
    #endif

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
        DUMP_TRACE("ldai");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    LDA_rANY:
    {
        GetAcc().Set(GetReg(decoder.GetFirstReg()));
        DUMP_TRACE("lda");
        ADVANCE_FETCH_AND_DISPATCH();

    }

    STA_aANY:
    {
        GetReg(decoder.GetFirstReg()).Set(GetAcc());
        DUMP_TRACE("sta");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    STNULL:
    {
        GetReg(decoder.GetFirstReg()).Set(Type::ANY, 0);
        DUMP_TRACE("stnull");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MOV_rANY:
    {
        GetReg(decoder.GetSecondReg()).Set(GetReg(decoder.GetFirstReg()));
        DUMP_TRACE("mov");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    // Handlers:
    JUMP:
    {
        pc_ += decoder.GetImm();
        ASSERT(decoder.GetImm() != 0);
        DUMP_TRACE("jump");
        FETCH_AND_DISPATCH();
    }
    
    BLE_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() <= 0.) {
            pc_ += decoder.GetImm();
            DUMP_TRACE("ble");
            FETCH_AND_DISPATCH();
        }
        DUMP_TRACE("ble");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    BLT_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() < 0.) {
            pc_ += decoder.GetImm();
            DUMP_TRACE("blt");
            FETCH_AND_DISPATCH();
        }
        DUMP_TRACE("blt");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    BGE_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() >= 0.) {
            pc_ += decoder.GetImm();
            DUMP_TRACE("bge");
            FETCH_AND_DISPATCH();
        }
        DUMP_TRACE("bge");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    BNE_aNUM:
    {
        ASSERT(decoder.GetImm() != 0);
        if (GetAcc().GetAsNum() != 0.) {
            pc_ += decoder.GetImm();
            DUMP_TRACE("bne");
            FETCH_AND_DISPATCH();
        }
        DUMP_TRACE("bne");
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
        DUMP_TRACE("ret");
        FETCH_AND_DISPATCH();
    }

    GETARG0: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetArg<0>());
        DUMP_TRACE("getarg0");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETARG1: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetArg<1>());
        DUMP_TRACE("getarg1");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETTHIS: {
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetThis());
        DUMP_TRACE("getthis");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    SETARG0_aFUNC_rANY: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        func_obj->SetArg<0>(GetReg(reg_id));
        DUMP_TRACE("setarg0");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETARG1_aFUNC_rANY: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        func_obj->SetArg<1>(GetReg(reg_id));
        DUMP_TRACE("setarg1");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    GETRET0_aFUNC: {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder.GetFirstReg();
        GetReg(reg_id).Set(*func_obj->GetRet<0>());
        DUMP_TRACE("getret0");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    SETRET0_rANY: {
        size_t reg_id = decoder.GetFirstReg();
        GetCallee()->SetRet<0>(GetReg(reg_id));
        DUMP_TRACE("setret0");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    CALL_aFUNC: {
        // return to the caller frame;
        // stack contains pc of the call instruction:
        auto *func_obj = GetAcc().GetAsFunction(); 
        Runtime::GetInterpreter()->GetStateStack()->emplace_back(pc_, func_obj);
        pc_ = func_obj->GetTargetPc();
        DUMP_TRACE("call");
        FETCH_AND_DISPATCH();
    }

    ADD_rNUM_rNUM: {
        GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() + GetReg(decoder.GetSecondReg()).GetAsNum());
        DUMP_TRACE("add");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB_rNUM_rNUM: {
        GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() - GetReg(decoder.GetSecondReg()).GetAsNum());
        DUMP_TRACE("sub");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV_rNUM_rNUM: {
        if (GetReg(decoder.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() / GetReg(decoder.GetSecondReg()).GetAsNum());
        }
        DUMP_TRACE("div");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MOD_rNUM_rNUM: {
        if (GetReg(decoder.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(std::fmod(GetReg(decoder.GetFirstReg()).GetAsNum(), GetReg(decoder.GetSecondReg()).GetAsNum()));
        }
        DUMP_TRACE("mod");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL_rNUM_rNUM: {
        GetAcc().Set(GetReg(decoder.GetFirstReg()).GetAsNum() * GetReg(decoder.GetSecondReg()).GetAsNum());
        DUMP_TRACE("mul");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    ADD_rSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        DUMP_TRACE("addstr");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aNUM_rNUM: {
        GetAcc().Set(GetAcc().GetAsNum() + GetReg(decoder.GetFirstReg()).GetAsNum());
        DUMP_TRACE("add2");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aNUM_rNUM: {
        GetAcc().Set(GetAcc().GetAsNum() - GetReg(decoder.GetFirstReg()).GetAsNum());
        DUMP_TRACE("sub2");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aNUM_rNUM: {
        if (GetReg(decoder.GetFirstReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(GetAcc().GetAsNum() / GetReg(decoder.GetFirstReg()).GetAsNum());
        }
        DUMP_TRACE("div2");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aNUM_rNUM: {
        GetAcc().Set(GetAcc().GetAsNum() * GetReg(decoder.GetFirstReg()).GetAsNum());
        DUMP_TRACE("mul2");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        DUMP_TRACE("add2");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SUB2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        DUMP_TRACE("sub2");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DIV2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        DUMP_TRACE("div2");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    MUL2_aARR_rNUM: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        DUMP_TRACE("mul2");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    ADD2_aSTR_rSTR: {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        DUMP_TRACE("add2");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    DECA_aNUM: {
        GetAcc().Set(GetAcc().GetAsNum() - 1.);
        DUMP_TRACE("deca");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    NEWARR_rNUM: {
        size_t reg_id = decoder.GetFirstReg();
        size_t arr_sz = static_cast<size_t>(GetReg(reg_id).GetAsNum());
        GetAcc().Set(coretypes::Array::New(Runtime::GetAllocator()->ObjectsRegion(), arr_sz));
        DUMP_TRACE("newarr");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETELEM_aANY_rARR_rNUM: {
        size_t idx = static_cast<size_t>(GetReg(decoder.GetSecondReg()).GetAsNum());
        GetReg(decoder.GetFirstReg()).GetAsArray()->SetElem(idx, GetAcc());
        DUMP_TRACE("setelem");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    SETELEM_aANY_rOBJ_rSTR: {
        auto string = GetReg(decoder.GetSecondReg()).GetAsString();
        GetReg(decoder.GetFirstReg()).GetAsObject()->SetElem(string->GetData(), GetAcc());
        DUMP_TRACE("setelem");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETELEM_rARR_rNUM: {
        size_t idx = static_cast<size_t>(GetReg(decoder.GetSecondReg()).GetAsNum());
        GetAcc().Set(*GetReg(decoder.GetFirstReg()).GetAsArray()->GetElem(idx));
        DUMP_TRACE("getelem");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    GETELEM_rOBJ_rSTR: {
        auto string = GetReg(decoder.GetSecondReg()).GetAsString();
        GetAcc().Set(*GetReg(decoder.GetFirstReg()).GetAsObject()->GetElem(string->GetData()));
        if (GetAcc().GetType() == Type::FUNC) {
            GetAcc().GetAsFunction()->SetThis(GetReg(decoder.GetFirstReg()));
        }
        DUMP_TRACE("getelem");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DUMP_rANY: {
        GetReg(decoder.GetFirstReg()).Dump();
        DUMP_TRACE("dump");
        ADVANCE_FETCH_AND_DISPATCH();
    }
    DUMPA_aANY: {
        GetAcc().Dump();
        DUMP_TRACE("dump");
        ADVANCE_FETCH_AND_DISPATCH();
    }

    trace_file_.close();
}

#undef FETCH_AND_DISPATCH

}  // namespace k3s 
