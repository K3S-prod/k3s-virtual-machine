#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "interpreter/generated/inst_decoder.h"
#include "common/macro.h"
#include <cstdint>
#include <cstdio>
#include <vector>

namespace k3s {

template <typename T>    
using Vector = std::vector<T>;

class AsmEncoder;
extern AsmEncoder ENCODER;

class AsmEncoder {
public:
    static int Process(FILE *file);

    template<size_t opc_size, size_t op1_size, size_t op2_size>
    void Encode(uint8_t opcode, uint8_t op1, uint8_t op2) {
        ASSERT((opc_size + op1_size + op2_size) == 16U);
        uint8_t operands = op2;
        operands <<= InstDecoder::SECOND_NEAR_REG_SHIFT;
        ASSERT(op1 <= InstDecoder::FIRST_NEAR_REG_MASK);
        operands |= op1;
        instructions_buffer_.emplace_back(opcode, operands);
    }

    template<size_t opc_size, size_t op1_size>
    void Encode(uint8_t opcode, uint8_t operand) {
        ASSERT((opc_size + op1_size) == 16U);
        instructions_buffer_.emplace_back(opcode, operand);
    }
    template<size_t opc_size>
    void Encode(uint8_t opcode) {
        uint8_t padding = 0;
        ASSERT((opc_size + sizeof(padding) * 8U) == 16U);
        instructions_buffer_.emplace_back(opcode, padding);
    }

    static auto &GetInstructionsBuffer() {
        return ENCODER.instructions_buffer_;
    }

private:
    Vector<BytecodeInstruction> instructions_buffer_ {};
};

} // namespace k3s

#endif  // ASSEMBLER_H
