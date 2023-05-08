#include "assembler.h"
#include "assembler/generated/grammar.l.hpp"
#include "common/macro.h"
#include "classfile/class_file.h"

#include <cstddef>
#include <cstdio>

int main(int argc, char *argv[])
{
    if (argc <= 2) {
        LOG_FATAL(ASSEMBLER, "Please, provide file");
    }
    auto file = std::fopen(argv[1], "r");
    if (file == nullptr) {
        LOG_FATAL(ASSEMBLER, "Error reading file: '" << argv[1] << "'");
    }

    if (k3s::AsmEncoder::Process(file) != 0) {
        LOG_FATAL(ASSEMBLER, "Parsing '" << argv[1] << "' failed");
    }

    if (argc >= 3) {
        auto out_file = std::fopen(argv[2], "wb");
        if (out_file == nullptr) {
            LOG_FATAL(ASSEMBLER, "Can't open file: " << argv[2] << "'");
        }
        k3s::AsmEncoder::DumpToFile(out_file);
        std::fclose(out_file);
    }

    return 0;
}

void k3s::AsmEncoder::DumpToFile(FILE *file) {
    ClassFile cf;
    return cf.DumpClassFile(file);
}
