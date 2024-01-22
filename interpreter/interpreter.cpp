#include "interpreter.h"
#include "types/coretypes.h"

namespace k3s {

std::ostream &operator<<(std::ostream &os, const BytecodeInstruction &inst)
{
    return *inst.Dump(&os);
}

void Interpreter::Invoke()
{
#ifdef TRACE
    auto *fn = std::getenv("K3S_TRACE");
    if (fn != nullptr) {
        trace_file_.open(fn);
        if (!trace_file_.is_open()) {
            LOG_FATAL(ASSEMBLER, "Can't open trace file");
        }
    }
#endif
    auto *main_ptr = coretypes::Function::New(Runtime::GetAllocator()->ObjectsRegion(), pc_);
    GetStateStack()->emplace_back(-1, main_ptr);

#include "generated/dispatch_table.inl"
}

}  // namespace k3s 
