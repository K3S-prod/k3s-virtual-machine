#include "classfile/class_file.h"
#include "assembler/assembler.h"
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
    auto main_id = ENCODER.TryResolveName(ENTRY_FUNC_NAME);
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
    size_t code_size = ENCODER.GetInstructionsBuffer().size() * sizeof(ENCODER.GetInstructionsBuffer()[0]);
    char * code_ptr = reinterpret_cast<char *>(ENCODER.GetInstructionsBuffer().data());
    write_buf(code_ptr, code_size);
}

void ClassFile::WriteConstantPool() 
{
    size_t pool_size = ENCODER.GetConstantPool().Elements().size();
    for (int8_t pool_id =0; pool_id < pool_size; ++pool_id) {
        WriteObj(ENCODER.GetConstantPool().GetElement(pool_id), pool_id);
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
    WriteConstantPool();
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

void ClassFile::WriteObj(const ConstantPool::Element &element, int8_t pool_id) 
{
    size_t obj_size = 0;
    MetaRecord meta = {
        .type = element.type_, 
        .size = EstimateEncodingSize(element) - sizeof(MetaRecord)
    };
    if (element.type_ == Register::Type::ANY) {
        return;
    }
    write_buf(reinterpret_cast<char *>(&meta), sizeof(meta));
    switch (element.type_) {
        case Register::Type::FUNC: {
            FuncRecord record = { .bc_offset = element.val_, .id = pool_id };
            ASSERT(sizeof(record) == meta.size);
            write_buf(reinterpret_cast<char *>(&record), sizeof(record));
            break;
        }
        case Register::Type::NUM: {
            NumRecord record = { .value = bit_cast<double>(element.val_), .id = pool_id };
            write_buf(reinterpret_cast<char *>(&record), sizeof(record));
            break;
        }
        default:
            // ASSERT(0 && "Unreachable: encoding of unsupported object type");
            break;
    }
}

void ClassFile::write_buf(char *src, size_t nbytes) 
{
    ASSERT(src && "Invalid pointer");
    char *buf_ptr = file_buffer.data() + buf_pos;
    std::memcpy(buf_ptr, src, nbytes);
    buf_pos += nbytes;
}

int ClassFile::LoadHeader(FILE *fileptr, ClassFileHeader *header) 
{
    size_t nread = std::fread(header, sizeof(*header), 1, fileptr);
    if (nread < 1) {
        return -1;
    }
    return 0;
}

int ClassFile::LoadCodeSection(FILE *fileptr, const ClassFileHeader &header,
                        Vector<BytecodeInstruction> *instructions_buffer_) 
{
    size_t code_size = header.table_offset - header.code_offset;
    ASSERT(code_size % sizeof(BytecodeInstruction) == 0 && "Invalid codesize");
    instructions_buffer_->resize(code_size / sizeof(BytecodeInstruction));
    size_t nbytes = std::fread(instructions_buffer_->data(), sizeof(char), 
                                                            code_size, fileptr);
    if (nbytes < code_size) {
        return -1;
    }
    return 0;
}

int ClassFile::LoadConstantPool(FILE *fileptr, ConstantPool *constant_pool)
{
    size_t cur_pos = std::ftell(fileptr);
    std::fseek(fileptr, 0, SEEK_END);
    size_t end_pos = std::ftell(fileptr);
    std::fseek(fileptr, cur_pos, SEEK_SET);
    size_t data_size = end_pos - cur_pos + 1;
    Vector<char> buffer(data_size);
    std::fread(buffer.data(), sizeof(buffer[0]), data_size, fileptr);
    for (size_t pos = 0; pos < buffer.size(); ) {
        MetaRecord meta = *((MetaRecord *)buffer.data());
        switch (meta.type) {
        case Register::Type::FUNC: {
            FuncRecord record;
            ASSERT(sizeof(record) == meta.size && "Invalid metadata");
            std::memcpy(&record, &buffer[pos], sizeof(record));
            pos += sizeof(record);
            constant_pool->SetFunction(record.id, record.bc_offset);
            break;
        }
        case Register::Type::NUM: {
            NumRecord record;
            ASSERT(sizeof(record) == meta.size && "Invalid metadata");
            std::memcpy(&record, &buffer[pos], sizeof(record));
            pos += sizeof(record);
            constant_pool->SetNum(record.id, record.value);
            break;
        }
        default:
            pos += meta.size;
            std::cerr << "Warning: trying to load unsupported type\n";
            break;
        }
    }
    return 0;
}

}; // namespace k3s