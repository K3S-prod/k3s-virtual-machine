#include "interpreter/interpreter.h"
#include "assembler/assembler.h"
#include "klee/klee.h"

// klee -link-llvm-lib=assembler/libencoder.a -link-llvm-lib=runtime/libruntime.a  -link-llvm-lib=allocator/libgc.a --libcxx --libc=klee --kdalloc-globals-start-address=704643072 --kdalloc-heap-start-address=184549376 testgen/klee/CMakeFiles/klee_sample.dir/main.cpp.o
int main ()
{
    k3s::Runtime::Create();
    k3s::Runtime::GetInterpreter()->GetStateStack()->emplace_back(0, nullptr);
    k3s::Runtime::GetInterpreter()->GetReg(0).Set(1.2);
    //k3s::AsmEncoder::Encode(k3s::Opcode::ADD, 0, 0);

    k3s::BytecodeInstruction bc_buf;
    klee_make_symbolic(&bc_buf.opcode_, sizeof(bc_buf.opcode_), "opcode");
    klee_make_symbolic(&bc_buf.operands_, sizeof(bc_buf.operands_), "operands");
    klee_assume(
        bc_buf.opcode_ < k3s::Opcode::OPCODE_LAST
    );

    k3s::Runtime::GetInterpreter()->SetProgram(&bc_buf);

#include "interpreter/generated/dispatch_table_single.inl"    
    std::cout << k3s::Runtime::GetInterpreter()->GetAcc().GetAsNum() << std::endl;
    return 0;
}