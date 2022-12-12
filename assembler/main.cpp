#include "assembler.h"
#include "assembler/generated/grammar.l.hpp"
#include "common/macro.h"

#include <cstddef>
#include <cstdio>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        LOG_FATAL(ASSEMBLER, "Please, provide file");
    }
    auto file = std::fopen(argv[1], "r");
    if (file == nullptr) {
        LOG_FATAL(ASSEMBLER, "Error reading file: '" << argv[1] << "'");
    }

    if (k3s::AsmEncoder::Process(file) != 0) {
        LOG_FATAL(ASSEMBLER, "Parsing '" << argv[1] << "' failed");
    }

    return 0;
}
