#pragma once

#include "runtime/runtime.h"
// TODO random inst generator
#include "interpreter/bytecode_instruction.h"

#include <array>
#include <vector>

namespace k3s {

namespace testgen {

struct StateImage {
    bool acc_image = false;
    std::array<bool, 16> regs_image;
};

class RuntimeImage {
public:
    RuntimeImage() {
        Runtime::Create();
        Runtime::GetInterpreter()->SetHookData(reinterpret_cast<void *>(this));
        program_.reserve(100);
    }

    auto Run() {
        state_stack_image_.emplace_back();
        Runtime::GetInterpreter()->SetPc(0);
        Runtime::GetInterpreter()->Invoke();
    }

    void Hook() {
        auto pc = Runtime::GetInterpreter()->GetPc();
        UpdateImage(pc);
        // In case of vector reallocation program pointer will dangle.
        Runtime::GetInterpreter()->SetProgram(program_.data());
    }

private:
    BytecodeInstruction GetNextInstruction() {
        // TODO: random inst generator
        return BytecodeInstruction(Opcode::RET, 0);
    }

    void CreateNumInConstantPool(uint8_t constant_pool_id, double value);
    void CreateStrInConstantPool(uint8_t constant_pool_id, const char *str);

    void CreateRandNumInConstantPool(uint8_t constant_pool_id);
    void CreateRandStrInConstantPool(uint8_t constant_pool_id);

    void UpdateImage(size_t pc);

    void UpdateProgramImage(size_t pc);

    void HandleReadAcc();

    void HandleReadAcc(Register::Type required_type);

    void HandleReadReg(size_t reg);

    void HandleReadReg(size_t reg, Register::Type required_type);

    void HandleWriteAcc();

    void HandleWriteReg(size_t reg);

private:
    static constexpr size_t SIZE_MATAINFO_CONST_POOL = 10000U;

    std::vector<BytecodeInstruction> program_{};
    std::vector<bool> program_image_{};
    std::array<bool, ConstantPool::CONSTANT_POOL_SIZE> const_pool_image_{};
    struct {
        size_t size = 0;
        std::array<char, SIZE_MATAINFO_CONST_POOL> data;
    } metainfo_constant_pool_{};
    std::vector<StateImage> state_stack_image_{};
};

} // namespace testgen

} // namespace k3s