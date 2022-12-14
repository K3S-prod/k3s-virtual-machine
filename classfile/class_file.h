#ifndef CLASSFILE_H
#define CLASSFILE_H

#include "common/macro.h"
#include "interpreter/types/coretypes.h"
#include "interpreter/bytecode_instruction.h"
#include <cstdint>
#include <array>
#include <vector>

namespace k3s {

template <typename T>
using Vector = std::vector<T>;

static constexpr const char *ENTRY_FUNC_NAME = "main";

class AsmEncoder;
extern AsmEncoder ENCODER;

class ConstantPool {
public:
    static constexpr size_t CONSTANT_POOL_SIZE = 256U;

    using Type = Register::Type;

    struct Element {
    public:
        Type type_ {Type::ANY};
        uint64_t val_ {};
    };

    void SetNum(uint8_t constant_pool_id, double num)
    {
        data_[constant_pool_id].type_ = Type::NUM;
        data_[constant_pool_id].val_ = bit_cast<uint64_t>(num);
    }
    
    size_t GetNum(uint8_t constant_pool_id)
    {
        ASSERT(data_[constant_pool_id].type_ == Type::NUM);
        return data_[constant_pool_id].val_;
    }

    void SetFunction(uint8_t constant_pool_id, size_t bytecode_ofs)
    {
        data_[constant_pool_id].type_ = Type::FUNC;
        data_[constant_pool_id].val_ = bytecode_ofs;
    }

    size_t GetFunctionBytecodeOffset(uint8_t constant_pool_id)
    {
        ASSERT(data_[constant_pool_id].type_ == Type::FUNC);
        return data_[constant_pool_id].val_;
    }
    
    const auto &GetElement(uint8_t constant_pool_id)
    {
        return data_[constant_pool_id];
    }

    const auto &Elements() 
    {
        return data_;
    }

private:
    std::array<Element, CONSTANT_POOL_SIZE> data_;
};

struct ClassFileHeader 
{
  uint64_t code_offset;
  uint64_t table_offset;
  uint64_t entry_point;
};

class ClassFile {
    Vector<char> file_buffer{0};
    size_t buf_pos;
public:
    struct MetaRecord {
        Register::Type type;    // following record type
        size_t size;            // following record size
    };
    struct FuncRecord {
        size_t bc_offset;
        int8_t id;
    };
    struct NumRecord {
        double value;
        int8_t id;
    };
    void DumpClassFile(FILE *fileptr);
    static size_t EstimateEncodingSize(const ConstantPool::Element &element);
    static int LoadHeader(FILE *fileptr, ClassFileHeader *header);
    static int LoadCodeSection(FILE *fileptr, const ClassFileHeader &header,
                        Vector<BytecodeInstruction> *instructions_buffer_);
    static int LoadConstantPool(FILE *fileptr, ConstantPool *constant_pool);
private:
    void AllocateBuffer();
    void WriteHeader();
    void WriteCodeSection();
    void WriteConstantPool();
    void WriteObj(const ConstantPool::Element &element, int8_t pool_id);
    void write_buf(char *src, size_t nbytes);
};

} // namespace k3s

#endif