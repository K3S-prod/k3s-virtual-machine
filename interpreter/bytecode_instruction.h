#ifndef INTERPRETER_BYTECODE_INSTRUCTION_H
#define INTERPRETER_BYTECODE_INSTRUCTION_H

#include "interpreter/generated/opcodes.h"
#include <iostream>

namespace k3s {

class BytecodeInstruction {
public:
    BytecodeInstruction(uint8_t opcode, uint8_t operands)
    : opcode_(static_cast<k3s::Opcode>(opcode)), operands_(operands) {}

    Opcode GetOpcode() const {
        return opcode_;
    }
    uint8_t GetOperands() const {
        return operands_;
    }

    void SetOperands(uint8_t operands) {
        operands_ = operands;
    }

    std::ostream *Dump(std::ostream *out) const
    {
        *out << "(opcode: " << static_cast<uint64_t>(opcode_) << ")\n";
        return out;
    }
private:
    Opcode opcode_ {};
    uint8_t operands_ {0};
};

std::ostream &operator<< (std::ostream &os, const BytecodeInstruction &inst);

using Immediate = int8_t;

static_assert(sizeof(BytecodeInstruction) == 2U);

}  // namespace k3s 

#endif  // INTERPRETER_BYTECODE_INSTRUCTION_H
