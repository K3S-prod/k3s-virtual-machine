#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "generated/opcodes.h"
#include <cstddef>
#include <cstdio>

class AsmEncoder {
public:
    template<size_t op1_size, size_t op2_size, size_t op3_size>
    void Encode(int op1, int op2, int op3) {
        printf("%d(%lu) %d(%lu) %d(%lu)\n", op1, op1_size, op2, op2_size, op3, op3_size);
    }

    template<size_t op1_size, size_t op2_size>
    void Encode(int op1, int op2) {
        printf("%d(%lu) %d(%lu)\n", op1, op1_size, op2, op2_size);
    }

private:

};

extern AsmEncoder encoder;

#endif  // ASSEMBLER_H
