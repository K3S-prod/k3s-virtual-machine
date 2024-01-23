#include "testgen/runtime_image.h"
#include <random>
#include "generated/inst_decoder.h"
#include "common/macro.h"

namespace k3s {

void Interpreter::FetchHook(void *rt_image) const {
    auto this_ = reinterpret_cast<testgen::RuntimeImage *>(rt_image);
    this_->Hook();
}

namespace testgen {

constexpr static float RAND_MIN_BOUND = -100000;
constexpr static float RAND_MAX_BOUND = 100000;

static double fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

static double GetRandNumObj() {
    return fRand(RAND_MIN_BOUND, RAND_MAX_BOUND);
}

static coretypes::String *GetRandStrObj() {
    // TODO really randomize string
    return coretypes::String::New(Runtime::GetAllocator()->ObjectsRegion(),
                                                                "random_str");
}

void RuntimeImage::CreateNumInConstantPool(uint8_t constant_pool_id, double value) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    Runtime::GetConstantPool()->SetNum(constant_pool_id, value);
}

void RuntimeImage::CreateRandNumInConstantPool(uint8_t constant_pool_id) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    auto value = fRand(RAND_MIN_BOUND, RAND_MAX_BOUND);
    Runtime::GetConstantPool()->SetNum(constant_pool_id, value);
}

void RuntimeImage::CreateRandStrInConstantPool(uint8_t constant_pool_id) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    size_t string_length = rand() % 100 + 1;
    ASSERT(string_length + metainfo_constant_pool_.size >= SIZE_MATAINFO_CONST_POOL);
    auto metainfo_string = metainfo_constant_pool_.data.data();
    auto start_string = metainfo_constant_pool_.size;
    for (size_t i = 0 ; i < string_length; i++) {
        metainfo_string[start_string + 1] = rand() % 127 + 1;
    }
    metainfo_string[start_string + string_length] = '\0';
    metainfo_constant_pool_.size += string_length + 1;

    Runtime::GetConstantPool()->SetStr(constant_pool_id, start_string);
}


void RuntimeImage::CreateStrInConstantPool(uint8_t constant_pool_id, const char *str) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    uint64_t string_start = reinterpret_cast<uint64_t>(metainfo_constant_pool_.data.data() + metainfo_constant_pool_.size);
    size_t string_length = strlen(str);
    ASSERT(string_length + metainfo_constant_pool_.size >= SIZE_MATAINFO_CONST_POOL);

    strcpy(metainfo_constant_pool_.data.data(), str);
    metainfo_constant_pool_.size += string_length + 1;

    Runtime::GetConstantPool()->SetStr(constant_pool_id, string_start);
}

void RuntimeImage::UpdateImage(size_t pc) {
    UpdateProgramImage(pc);
    const auto &inst = program_[pc];
    auto opc = inst.GetOpcode();
    InstDecoder decoder;
    decoder.Decode(inst);

    switch (opc) {
        case Opcode::BLE:
        case Opcode::BLT:
        case Opcode::BGE:
        case Opcode::BNE:
            HandleReadAcc(Register::Type::NUM);
            break;
        case Opcode::CALL:
            HandleReadAcc(Register::Type::FUNC);
            state_stack_image_.emplace_back();
            break;
        case Opcode::RET:
            state_stack_image_.pop_back();
            break;
        case Opcode::GETTHIS:
        case Opcode::STNULL:
            HandleWriteReg(decoder.GetFirstReg());
            break;
        case Opcode::LDAI:
            // TODO what type entry should be?
            if (!const_pool_image_[decoder.GetImm()]) {
                CreateRandNumInConstantPool(decoder.GetImm());
                const_pool_image_[decoder.GetImm()] = true;
            }
            HandleWriteAcc();
            break;
        case Opcode::LDA:
            HandleReadReg(decoder.GetFirstReg());
            HandleWriteAcc();
            break;
        case Opcode::STA:
            HandleReadAcc();
            HandleWriteReg(decoder.GetFirstReg());
            break;
        case Opcode::MOV:
            HandleReadReg(decoder.GetFirstReg());
            HandleWriteReg(decoder.GetSecondReg());
            break;
        case Opcode::ADD:
            // TODO handle overloads. Need type hint from random generator.
            HandleReadReg(decoder.GetFirstReg(), Register::Type::NUM);
            HandleReadReg(decoder.GetSecondReg(), Register::Type::NUM);
            HandleWriteAcc();
            break;
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::MOD:
            HandleReadReg(decoder.GetFirstReg(), Register::Type::NUM);
            HandleReadReg(decoder.GetSecondReg(), Register::Type::NUM);
            HandleWriteAcc();
            break;
        case Opcode::ADD2:
            // TODO handle overloads. Need type hint from random generator.
            HandleReadReg(decoder.GetFirstReg(), Register::Type::NUM);
            HandleReadAcc(Register::Type::NUM);
            break;
        case Opcode::SUB2:
        case Opcode::MUL2:
        case Opcode::DIV2:
            HandleReadReg(decoder.GetFirstReg(), Register::Type::NUM);
            HandleReadAcc(Register::Type::NUM);
            break;
        case Opcode::DECA:
            HandleReadAcc(Register::Type::NUM);
            break;
        case Opcode::NEWARR:
            HandleReadReg(decoder.GetFirstReg(), Register::Type::NUM);
            HandleWriteAcc();
            break;
        case Opcode::SETELEM:
            HandleReadAcc();
            HandleReadReg(decoder.GetFirstReg(), Register::Type::ARR);
            HandleReadReg(decoder.GetSecondReg(), Register::Type::NUM);
            // TODO handle objects
            break;
        // TODO other opcodes
        default:
            break;
    }
}

void RuntimeImage::UpdateProgramImage(size_t pc) {
    size_t prog_size = program_.size();
    if (pc >= prog_size) {
        program_.resize(pc + prog_size / 2);
        program_image_.resize(pc + prog_size / 2, false);
    }
    if (!program_image_[pc]) {
        program_[pc] = GetNextInstruction();
        program_image_[pc] = true;
    }
    program_image_[pc] = true;
}

void RuntimeImage::HandleReadAcc() {
    if (!state_stack_image_.back().acc_image) {
        Runtime::GetInterpreter()->GetAcc().Set(GetRandNumObj());
    }
    state_stack_image_.back().acc_image = true;
}

void RuntimeImage::HandleReadAcc(Register::Type required_type) {
    if (state_stack_image_.back().acc_image) {
        if (required_type != Runtime::GetInterpreter()->GetAcc().GetType()) {
            LOG_FATAL("runtime_image", "read ACC type mismatch");
        }
        return;
    }
    switch (required_type) {
        case Register::Type::NUM:
        Runtime::GetInterpreter()->GetAcc().Set(GetRandNumObj());
        break;
        case Register::Type::STR:
        Runtime::GetInterpreter()->GetAcc().Set(GetRandStrObj());
        break;
        default:
        // TODO other types
        break;
    }
    state_stack_image_.back().acc_image = true;
}

void RuntimeImage::HandleReadReg(size_t reg) {
    if (!state_stack_image_.back().acc_image) {
        Runtime::GetInterpreter()->GetReg(reg).Set(GetRandNumObj());
    }
    state_stack_image_.back().acc_image = true;
}

void RuntimeImage::HandleReadReg(size_t reg, Register::Type required_type) {
    auto &regval = Runtime::GetInterpreter()->GetReg(reg);
    if (state_stack_image_.back().regs_image[reg]) {
        if (required_type != regval.GetType()) {
            LOG_FATAL("runtime_image", "read reg type mismatch");
        }
        return;
    }
    switch (required_type) {
        case Register::Type::NUM:
        regval.Set(GetRandNumObj());
        break;
        case Register::Type::STR:
        regval.Set(GetRandStrObj());
        break;
        default:
        // TODO other types
        break;
    }
    state_stack_image_.back().regs_image[reg] = true;
}

void RuntimeImage::HandleWriteAcc() {
    state_stack_image_.back().acc_image = true;
}

void RuntimeImage::HandleWriteReg(size_t reg) {
    state_stack_image_.back().regs_image[reg] = true;
}

} // namespace testgen

} // namespace k3s
