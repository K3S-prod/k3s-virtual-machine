#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include "allocator/allocator.h"
#include "allocator/gc.h"
#include "classfile/class_file.h"

namespace k3s {

class Runtime;
class Interpreter;

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
        return GetInstance()->interpreter_;
    }

    static auto *GetAllocator()
    {
        return &GetInstance()->allocator_;
    }

    static auto *GetGC()
    {
        return &GetInstance()->gc_;
    }

    static int LoadClassFile(const char *fn);

private:
    Allocator allocator_ {};
    GC gc_{};
    Interpreter *interpreter_ {};
    ConstantPool constant_pool_ {};
};

}  // namespace k3s

#endif
