#include "classfile/class_file.h"
#include <vector>
#include <fstream>
#include <bit>

namespace k3s {

void ClassFile::AllocateBuffer() 
{
    size_t file_size = sizeof(ClassFileHeader);
    file_size +=
        ENCODER.GetInstructionsBuffer().size() * sizeof(BytecodeInstruction);
    for (auto pool_element : ENCODER.GetConstantPool().Elements()) {
        file_size += EstimateEncodingSize(pool_element);
    }
    file_buffer.resize(file_size);
    buf_pos = 0;
}

void ClassFile::WriteHeader() 
{
    ClassFileHeader header;
    size_t header_size = sizeof(header);
    header.code_offset = header_size;
    auto main_id = ENCODER.ResolveName(ENTRY_FUNC_NAME);
    header.entry_point =
        ENCODER.GetConstantPool().GetFunctionBytecodeOffset(main_id);
    header.table_offset =
        header.code_offset +
        ENCODER.GetInstructionsBuffer().size() * sizeof(BytecodeInstruction);
    write_buf(reinterpret_cast<char *>(&header), header_size);
    ASSERT(buf_pos == header_size && "Trash header write");
}

void ClassFile::WriteCodeSection() 
{
    size_t code_size = ENCODER.GetInstructionsBuffer().size();
    char * code_ptr = reinterpret_cast<char *>(ENCODER.GetInstructionsBuffer().data());
    write_buf(code_ptr, code_size);
}

void ClassFile::WriteConstantTable() 
{
    for (const auto &element : ENCODER.GetConstantPool().Elements()) {
        WriteObj(element);
    }
}

void ClassFile::DumpClassFile(FILE *fileptr)
{
    ASSERT(fileptr);
    AllocateBuffer();
    WriteHeader();
    ASSERT(buf_pos < file_buffer.size() && "Invalid header write");
    WriteCodeSection();
    ASSERT(buf_pos < file_buffer.size() && "Invalid code write");
    WriteConstantTable();
    ASSERT(buf_pos <= file_buffer.size() && "Invalid table write");
    std::fwrite(file_buffer.data(), sizeof(file_buffer[0]), file_buffer.size(), fileptr);
}

size_t ClassFile::EstimateEncodingSize(const ConstantPool::Element &element) 
{
    size_t size = sizeof(MetaRecord);
    switch (element.type_) {
        case Register::Type::FUNC:
            size += sizeof(FuncRecord);
            break;
        case Register::Type::NUM:
            size += sizeof(NumRecord);
            break;
        case Register::Type::ANY:
            size = 0;
            break;
        default:
            std::cerr << "Warning: calculating size of unsupported type.\n";
            size = 0;
    }
    return size;
}

void ClassFile::WriteObj(const ConstantPool::Element &element) 
{
    size_t obj_size = 0;
    MetaRecord meta = {
        .type = element.type_, 
        .size = EstimateEncodingSize(element)
    };
    if (element.type_ == Register::Type::ANY) {
        return;
    }
    write_buf(reinterpret_cast<char *>(&meta), sizeof(meta));
    switch (element.type_) {
        case Register::Type::FUNC: {
            FuncRecord record = { .bc_offset = element.val_ };
            write_buf(reinterpret_cast<char *>(&record), sizeof(record));
            break;
        }
        case Register::Type::NUM: {
            NumRecord record = { .value = bit_cast<double>(element.val_) };
            write_buf(reinterpret_cast<char *>(&record), sizeof(record));
            break;
        }
        default:
            // ASSERT(0 && "Unreachable: encoding of unsupported object type");
            break;
    }
}

void ClassFile::write_buf(char *src, size_t nbytes) {
    ASSERT(src && "Invalid pointer");
    char *buf_ptr = file_buffer.data() + buf_pos;
    std::memcpy(buf_ptr, src, nbytes);
    buf_pos += nbytes;
}

}; // namespace k3s