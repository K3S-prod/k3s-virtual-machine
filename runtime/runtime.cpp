#include "runtime/runtime.h"
#include "allocator/allocator.h"

namespace k3s {

Runtime *RUNTIME;

void Runtime::Create()
{
    Allocator a;
    a.Init();
    RUNTIME = new (a.ConstRegion().Alloc<Runtime>(1)) Runtime();
}

}  // namespace k3s
