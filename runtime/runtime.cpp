#include "runtime/runtime.h"
#include "interpreter/interpreter.h"
#include "allocator/allocator.h"

namespace k3s {

Runtime *RUNTIME;

void Runtime::Create()
{
    Allocator a;
    a.Init();
    RUNTIME = new (a.ConstRegion().Alloc<Runtime>(1)) Runtime();
    RUNTIME->interpreter_ = new (a.ConstRegion().Alloc<Interpreter>(1)) Interpreter();
}

int Runtime::LoadClassFile(const char *fn)
{
    ClassFileHeader *header;
    BytecodeInstruction *instructions_buffer;

    int err_code = ClassFile::LoadClassFile(fn, &header, 
                                        &instructions_buffer, GetConstantPool(), GetAllocator());
    if (err_code != 0) {
        return err_code;
    }
    GetInterpreter()->SetPc(header->entry_point);
    GetInterpreter()->SetProgram(instructions_buffer);
    return 0;
}

}  // namespace k3s
