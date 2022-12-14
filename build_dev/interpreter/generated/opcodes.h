#ifndef OPCODES_H
#define OPCODES_H

#include <cstdint>

namespace k3s {

enum class Opcode : uint8_t {
    // Controlflow
        // opc_i8
        JUMP,
        // opc_i8
        BLE,
        // opc
        CALL,
        // opc
        RET,
    // Objects
        // opc_i8
        LDAI,
    // DataFlow
        // opc_r8
        LDA,
        // opc_r8
        STA,
        // opc_r4_r4
        MOV,
    // Arithmetic (two-op)
        // opc_r4_r4
        ADD,
        // opc_r4_r4
        SUB,
        DIV,
        MUL,
    // Arithmetic (single-op)
        // opc_r8
        ADD2,
        // opc_r8
        SUB2,
        DIV2,
        MUL2,
    // Arithmetic (zero-op)
        // opc
        DECA,
    // Array-specific
        // opc_r8
        NEWARR,
        // opc_r4_r4
        SETELEM,
        // opc_r4_r4
        GETELEM,
    // Dump
        // opc_r8
        DUMP,
        // opc
        DUMPA,
    // Function-specific
        // opc_r8
        GETARG0,
        SETRET0,
        // opc_r8
        SETARG0,
        GETRET0,

};

}  // namespace k3s

#endif  // OPCODES_H
