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
  uint64_t code_offset;     // File offset in bytes to start of bytecode instructions
  uint64_t table_offset;    // File offset in bytes to start of constant pool
  uint64_t entry_point;     // Bytecode offset of main function
};

/// Class representing classfile format. 
/// File format grammar: 
/// File : Header Code ConstPool
/// Header : ClassFileHeader
/// Code : BytecodeInstructions*
/// ConstPool : (MetaRecord Record)*
/// Record : NumRecord | FuncRecord
/// Recrod structure desribed below
class ClassFile {
    Vector<char> file_buffer_;
    size_t buf_pos_;
public:
    struct MetaRecord {
        Register::Type type;    // following record type
    };
    struct FuncRecord {
        size_t bc_offset;
        int8_t id;
    };
    struct NumRecord {
        double value;
        int8_t id;
    };
    /// Write classfile to \p fileptr
    void DumpClassFile(FILE *fileptr);
    /// Load classfile from \p fileptr
    static int LoadClassFile(FILE *fileptr, ClassFileHeader *header,
                            Vector<BytecodeInstruction> *instructions_buffer,
                            ConstantPool *const_pool);

private:
    /// Loads classfile to \p header header from \p fileptr
    static int LoadHeader(FILE *fileptr, ClassFileHeader *header);
    /// Loads code section to \p instructions_buffer_ from \p fileptr
    static int LoadCodeSection(FILE *fileptr, const ClassFileHeader &header,
                        Vector<BytecodeInstruction> *instructions_buffer);
    /// Loads constant pool to \p constant_pool from \p fileptr
    static int LoadConstantPool(FILE *fileptr, ConstantPool *constant_pool);
    static size_t EstimateEncodingSize(const ConstantPool::Element &element);
    void AllocateBuffer();
    /// Interfaces foe writing classfile parts to file
    void WriteHeader();
    void WriteCodeSection();
    void WriteConstantPool();
    void WriteObj(const ConstantPool::Element &element, int8_t pool_id);
    void write_buf(char *src, size_t nbytes);
};

} // namespace k3s

#endif