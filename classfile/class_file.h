#ifndef CLASSFILE_H
#define CLASSFILE_H

#include "common/macro.h"
#include "assembler/assembler.h"
#include <cstdint>

namespace k3s {

static constexpr const char *ENTRY_FUNC_NAME = "main";

struct ClassFileHeader 
{
  uint64_t code_offset;
  uint64_t table_offset;
  uint64_t entry_point;
};

class ClassFile {
    std::vector<char> file_buffer{0};
    size_t buf_pos;
public:
    struct MetaRecord {
        Register::Type type;    // following record type
        size_t size;            // following record size
    };
    struct FuncRecord {
        size_t bc_offset;
    };
    struct NumRecord {
        double value;
    };
    void DumpClassFile(FILE *fileptr);
    static size_t EstimateEncodingSize(const ConstantPool::Element &element);
private:
    void AllocateBuffer();
    void WriteHeader();
    void WriteCodeSection();
    void WriteConstantTable();
    void WriteObj(const ConstantPool::Element &element);
    void write_buf(char *src, size_t nbytes);
};

} // namespace k3s

#endif