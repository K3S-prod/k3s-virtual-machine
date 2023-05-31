#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include "allocator/allocator.h"
#include "allocator/gc.h"
#include "interpreter/interpreter.h"
#include "classfile/class_file.h"

namespace k3s {

class Runtime;

extern Runtime *RUNTIME;

class Runtime
{
public:
    static void Create();
    static Runtime *GetInstance()
    {
        return RUNTIME;
    }

    static auto *GetConstantPool()
    {
        return &GetInstance()->constant_pool_;
    }

    static auto *GetInterpreter()
    {
        return &GetInstance()->interpreter_;
    }

    static auto *GetAllocator()
    {
        return &GetInstance()->allocator_;
    }

    static auto *GetGC()
    {
        return &GetInstance()->gc_;
    }

    static int LoadClassFile(const char *fn)
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

private:
    Allocator allocator_ {};
    GC gc_{};
    Interpreter interpreter_ {};
    ConstantPool constant_pool_ {};
};

}  // namespace k3s

#endif
