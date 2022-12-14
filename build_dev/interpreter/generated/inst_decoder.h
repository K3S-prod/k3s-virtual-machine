#ifndef INTERPRETER_INST_DECODER
#define INTERPRETER_INST_DECODER

#include <cstdint>
#include <cstddef>
#include "interpreter/bytecode_instruction.h"

namespace k3s {

class Interpreter;

/**
 * Decode instruction.
 * 
 * This class is intended to decode instruction, store arguments for the pending instruction and
 * to give idx in the dispatch table.
 * 
 * After decoding, this class holds valid operands according to the signature of the opcode.
 *
 * In fact, this should be generated based on isa for all possible signatures.
 */
class InstDecoder {
public:
    static constexpr uint8_t FIRST_NEAR_REG_MASK = 15U;  
    static constexpr uint8_t SECOND_NEAR_REG_MASK = 255U - 15U;
    static constexpr uint8_t SECOND_NEAR_REG_SHIFT = 4U;
    static constexpr uint8_t FIRST_FAR_REG_MASK = 255U;
    static constexpr uint8_t IMM_MASK = 255U;
    static constexpr uint8_t OPCODE_SIZE_BITS = 8U;
    static constexpr uint8_t MAX_OPC_OVERLOAD_SIZE_BITS = 2U;
    static constexpr uint8_t MAX_OPC_OVERLOADS = 1U << MAX_OPC_OVERLOAD_SIZE_BITS;
    static constexpr size_t INVALID_OVERLOAD_IDX = -1;

    static_assert(4 == MAX_OPC_OVERLOADS);

    size_t GetFirstReg() {
        return register_operands_idx_[0];
    }
    size_t GetSecondReg() {
        return register_operands_idx_[1];
    }
    int8_t GetImm() {
        return immediate_operand_;
    }

    size_t DecodeAndResolve(const BytecodeInstruction &inst, const Interpreter &interp);

    Opcode Decode(const BytecodeInstruction &inst);
private:
    size_t register_operands_idx_[2];
    uint8_t immediate_operand_;
};

}

#endif  // INTERPRETER_INST_DECODER
