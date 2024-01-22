
    void LDAI() {
        const auto &elem = Runtime::GetConstantPool()->GetElement(decoder_.GetImm());
        switch (elem.type_) {
            case Type::FUNC:{
                size_t bc_offs = Runtime::GetConstantPool()->GetFunctionBytecodeOffset(decoder_.GetImm());
                auto *ptr = coretypes::Function::New(Runtime::GetAllocator()->ObjectsRegion(), bc_offs);
                GetAcc().Set(ptr);
                break;
            } case Type::NUM:{
                GetAcc().Set(bit_cast<double>(elem.val_)); 
                break;
            } case Type::STR:{
                auto *ptr = coretypes::String::New(Runtime::GetAllocator()->ObjectsRegion(), reinterpret_cast<const char *>(elem.val_));
                GetAcc().Set(ptr); 
                break;
            } case Type::OBJ:
            {
                auto *ptr = coretypes::Object::New(Runtime::GetAllocator()->ObjectsRegion(), *Runtime::GetConstantPool()->GetMappingForObjAt(decoder_.GetImm()), reinterpret_cast<size_t *>(elem.val_));
                GetAcc().Set(ptr); 
                break;
            }
            default:{
                LOG_FATAL(INTERPRETER, "Unknown constant pool element type for index (i = " << static_cast<int64_t>(decoder_.GetImm()) << ")" );
            }
        }
        pc_++;
    }
    void LDA_rANY() {
        GetAcc().Set(GetReg(decoder_.GetFirstReg()));
        pc_++;
    }
    void STA_aANY() {
        GetReg(decoder_.GetFirstReg()).Set(GetAcc());
        pc_++;
    }
    void STNULL() {
        GetReg(decoder_.GetFirstReg()).Set(Type::ANY, 0);
        pc_++;
    }
    void MOV_rANY() {
        GetReg(decoder_.GetSecondReg()).Set(GetReg(decoder_.GetFirstReg()));
        pc_++;
    }
    void JUMP() {
        pc_ += decoder_.GetImm();
        ASSERT(decoder_.GetImm() != 0);
    }
    void BLE_aNUM() {
        ASSERT(decoder_.GetImm() != 0);
        if (GetAcc().GetAsNum() <= 0.) {
            pc_ += decoder_.GetImm();
        } else {
            pc_++;
        }
    }
    void BLT_aNUM() {
        ASSERT(decoder_.GetImm() != 0);
        if (GetAcc().GetAsNum() < 0.) {
            pc_ += decoder_.GetImm();
        } else {
            pc_++;
        }
    }
    void BGE_aNUM() {
        ASSERT(decoder_.GetImm() != 0);
        if (GetAcc().GetAsNum() >= 0.) {
            pc_ += decoder_.GetImm();
        } else {
            pc_++;
        }
    }
    void BNE_aNUM() {
        ASSERT(decoder_.GetImm() != 0);
        if (GetAcc().GetAsNum() != 0.) {
            pc_ += decoder_.GetImm();
        } else {
            pc_++;
        }
    } 
    void RET() {
        if (Runtime::GetInterpreter()->GetStateStack()->size() == 1) {
            // TODO: replace to proper impl:
            if (trace_file_) {
                trace_file_.close();
            }
            std::exit(0);
        } else {
            // return to the caller frame;
            // stack contains pc of the call instruction:
            pc_ = Runtime::GetInterpreter()->GetStateStack()->back().caller_pc_;
            pc_++;
            Runtime::GetInterpreter()->GetStateStack()->pop_back();
        }
    }

    void GETARG0() {
        size_t reg_id = decoder_.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetArg<0>());
        pc_++;
    }
    void GETARG1() {
        size_t reg_id = decoder_.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetArg<1>());
        pc_++;
    }
    void GETTHIS() {
        size_t reg_id = decoder_.GetFirstReg();
        GetReg(reg_id).Set(*GetCallee()->GetThis());
        pc_++;
    }

    void SETARG0_aFUNC_rANY() {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder_.GetFirstReg();
        func_obj->SetArg<0>(GetReg(reg_id));
        pc_++;
    }
    void SETARG1_aFUNC_rANY() {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder_.GetFirstReg();
        func_obj->SetArg<1>(GetReg(reg_id));
        pc_++;
    }

    void GETRET0_aFUNC() {
        auto *func_obj = bit_cast<coretypes::Function *>(GetAcc().GetValue());
        size_t reg_id = decoder_.GetFirstReg();
        GetReg(reg_id).Set(*func_obj->GetRet<0>());
        pc_++;
    }

    void SETRET0_rANY() {
        size_t reg_id = decoder_.GetFirstReg();
        GetCallee()->SetRet<0>(GetReg(reg_id));
        pc_++;
    }

    void CALL_aFUNC() {
        // return to the caller frame;
        // stack contains pc of the call instruction:
        auto *func_obj = GetAcc().GetAsFunction(); 
        Runtime::GetInterpreter()->GetStateStack()->emplace_back(pc_, func_obj);
        pc_ = func_obj->GetTargetPc();
    }

    void ADD_rNUM_rNUM() {
        GetAcc().Set(GetReg(decoder_.GetFirstReg()).GetAsNum() + GetReg(decoder_.GetSecondReg()).GetAsNum());
        pc_++;
    }
    void SUB_rNUM_rNUM() {
        GetAcc().Set(GetReg(decoder_.GetFirstReg()).GetAsNum() - GetReg(decoder_.GetSecondReg()).GetAsNum());
        pc_++;
    }
    void DIV_rNUM_rNUM() {
        if (GetReg(decoder_.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(GetReg(decoder_.GetFirstReg()).GetAsNum() / GetReg(decoder_.GetSecondReg()).GetAsNum());
        }
        pc_++;
    }
    void MOD_rNUM_rNUM() {
        if (GetReg(decoder_.GetSecondReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(std::fmod(GetReg(decoder_.GetFirstReg()).GetAsNum(), GetReg(decoder_.GetSecondReg()).GetAsNum()));
        }
        pc_++;
    }
    void MUL_rNUM_rNUM() {
        GetAcc().Set(GetReg(decoder_.GetFirstReg()).GetAsNum() * GetReg(decoder_.GetSecondReg()).GetAsNum());
        pc_++;
    }
    void ADD_rSTR_rSTR() {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        pc_++;
    }

    void ADD2_aNUM_rNUM() {
        GetAcc().Set(GetAcc().GetAsNum() + GetReg(decoder_.GetFirstReg()).GetAsNum());
        pc_++;
    }
    void SUB2_aNUM_rNUM() {
        GetAcc().Set(GetAcc().GetAsNum() - GetReg(decoder_.GetFirstReg()).GetAsNum());
        pc_++;
    }
    void DIV2_aNUM_rNUM() {
        if (GetReg(decoder_.GetFirstReg()).GetAsNum() == 0) {
            LOG_FATAL(RUNTIME_ERROR, "Division by Zero");
        } else {
            GetAcc().Set(GetAcc().GetAsNum() / GetReg(decoder_.GetFirstReg()).GetAsNum());
        }
        pc_++;
    }
    void MUL2_aNUM_rNUM() {
        GetAcc().Set(GetAcc().GetAsNum() * GetReg(decoder_.GetFirstReg()).GetAsNum());
        pc_++;
    }

    void ADD2_aARR_rNUM() {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        pc_++;
    }
    void SUB2_aARR_rNUM() {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        pc_++;
    }
    void DIV2_aARR_rNUM() {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        pc_++;
    }
    void MUL2_aARR_rNUM() {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        pc_++;
    }

    void ADD2_aSTR_rSTR() {
        LOG_FATAL(INTERPERTER, "opc overload is unimplemented");
        pc_++;
    }

    void DECA_aNUM() {
        GetAcc().Set(GetAcc().GetAsNum() - 1.);
        pc_++;
    }

    void NEWARR_rNUM() {
        size_t reg_id = decoder_.GetFirstReg();
        size_t arr_sz = static_cast<size_t>(GetReg(reg_id).GetAsNum());
        GetAcc().Set(coretypes::Array::New(Runtime::GetAllocator()->ObjectsRegion(), arr_sz));
        pc_++;
    }
    void SETELEM_aANY_rARR_rNUM() {
        size_t idx = static_cast<size_t>(GetReg(decoder_.GetSecondReg()).GetAsNum());
        GetReg(decoder_.GetFirstReg()).GetAsArray()->SetElem(idx, GetAcc());
        pc_++;
    }
    void SETELEM_aANY_rOBJ_rSTR() {
        auto string = GetReg(decoder_.GetSecondReg()).GetAsString();
        GetReg(decoder_.GetFirstReg()).GetAsObject()->SetElem(string->GetData(), GetAcc());
        pc_++;
    }
    void GETELEM_rARR_rNUM() {
        size_t idx = static_cast<size_t>(GetReg(decoder_.GetSecondReg()).GetAsNum());
        GetAcc().Set(*GetReg(decoder_.GetFirstReg()).GetAsArray()->GetElem(idx));
        pc_++;
    }
    void GETELEM_rOBJ_rSTR() {
        auto string = GetReg(decoder_.GetSecondReg()).GetAsString();
        GetAcc().Set(*GetReg(decoder_.GetFirstReg()).GetAsObject()->GetElem(string->GetData()));
        if (GetAcc().GetType() == Type::FUNC) {
            GetAcc().GetAsFunction()->SetThis(GetReg(decoder_.GetFirstReg()));
        }
        pc_++;
    }
    void DUMP_rANY() {
        GetReg(decoder_.GetFirstReg()).Dump();
        pc_++;
    }
    void DUMPA_aANY() {
        GetAcc().Dump();
        pc_++;
    }
