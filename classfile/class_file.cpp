#include "classfile/class_file.h"
#include "assembler/assembler.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <fstream>

namespace k3s {

void ClassFile::AllocateBuffer() 
{
    size_t file_size = sizeof(ClassFileHeader);
    file_size +=
        ENCODER.GetInstructionsBuffer().size() * sizeof(BytecodeInstruction);
    for (auto pool_element : ENCODER.GetConstantPool().Elements()) {
        file_size += EstimateEncodingSize(pool_element);
    }
    file_buffer_.resize(file_size);
    buf_pos_ = 0;
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
    ASSERT(buf_pos_ == header_size && "Trash header write");
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
    ASSERT(buf_pos_ < file_buffer_.size() && "Invalid header write");
    WriteCodeSection();
    ASSERT(buf_pos_ < file_buffer_.size() && "Invalid code write");
    WriteConstantPool();
    ASSERT(buf_pos_ <= file_buffer_.size() && "Invalid table write");
    std::fwrite(file_buffer_.data(), sizeof(file_buffer_[0]), file_buffer_.size(), fileptr);
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
        .type = element.type_
    };
    if (element.type_ == Register::Type::ANY) {
        return;
    }
    write_buf(reinterpret_cast<char *>(&meta), sizeof(meta));
    switch (element.type_) {
        case Register::Type::FUNC: {
            FuncRecord record = { .bc_offset = element.val_, .id = pool_id };
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
    char *buf_ptr = file_buffer_.data() + buf_pos_;
    std::memcpy(buf_ptr, src, nbytes);
    buf_pos_ += nbytes;
}

size_t GetFileSize(int fd)
{
    auto pos = lseek(fd, 0, SEEK_CUR);
    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, pos, SEEK_SET);
    return size;
}
void *ReadFile(int fd, size_t size, Allocator *allocator)
{
    auto buf = allocator->ConstRegion().AllocBytes(size);
    auto ret = read(fd, buf, size);
    ASSERT(ret == size);
    return buf;
}

int ClassFile::LoadClassFile(const char *fn, ClassFileHeader **header,
                            BytecodeInstruction **instr_buffer,
                            ConstantPool *const_pool, Allocator *allocator) 
{
    int fd = open(fn, O_RDONLY);
    ASSERT(fd != -1);
    auto file_size = GetFileSize(fd);
    auto filebuf = ReadFile(fd, file_size, allocator);
    close(fd);
    
    *header = ClassFile::LoadHeader(filebuf);

    size_t code_size = (*header)->table_offset - (*header)->code_offset;
    ASSERT(code_size % sizeof(BytecodeInstruction) == 0 && "Invalid codesize");
    *instr_buffer = ClassFile::LoadCodeSection(filebuf);


    int err_code = ClassFile::LoadConstantPool(filebuf + sizeof(ClassFileHeader) + code_size, file_size -  sizeof(ClassFileHeader) - code_size,  const_pool);

    return err_code;
}

ClassFileHeader *ClassFile::LoadHeader(void *fileptr) 
{
    return reinterpret_cast<ClassFileHeader*>(fileptr);
}

BytecodeInstruction *ClassFile::LoadCodeSection(void *filebuf) 
{
    return reinterpret_cast<BytecodeInstruction *>(filebuf + sizeof(ClassFileHeader));
}

int ClassFile::LoadConstantPool(void *constpool_file, size_t bytes_count, ConstantPool *constant_pool)
{
    for (size_t pos = 0; pos < bytes_count; ) {
        MetaRecord meta;
        std::memcpy(&meta, constpool_file + pos, sizeof(meta));
        pos += sizeof(meta);
        switch (meta.type) {
        case Register::Type::FUNC: {
            FuncRecord record;
            std::memcpy(&record, constpool_file + pos, sizeof(record));
            pos += sizeof(record);
            constant_pool->SetFunction(record.id, record.bc_offset);
            break;
        }
        case Register::Type::NUM: {
            NumRecord record;
            std::memcpy(&record, constpool_file + pos, sizeof(record));
            pos += sizeof(record);
            constant_pool->SetNum(record.id, record.value);
            break;
        }
        default:
            std::cerr << "Unreachable executed: trying to load unsupported type\n";
            std::exit(EXIT_FAILURE);
            break;
        }
    }
    return 0;
}

}; // namespace k3s