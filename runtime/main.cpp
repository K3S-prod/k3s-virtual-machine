#include "runtime/runtime.h"
#include "allocator/allocator.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 1;
    }

    k3s::Runtime::Create();
    
    if (k3s::Runtime::LoadClassFile(argv[1]) != 0) {
        LOG_FATAL(INTERPRETER, "Loading failed");
    }

    k3s::Runtime::GetInterpreter()->Invoke();

    return 0;
}
