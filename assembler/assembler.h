#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "interpreter/generated/inst_decoder.h"
#include "interpreter/types/coretypes.h"
#include "common/macro.h"
#include <cstdint>
#include <cstdio>
#include <vector>
#include <unordered_map>

namespace k3s {

template <typename T>    
using Vector = std::vector<T>;
template <typename K, typename T>
using Hash = std::unordered_map<K, T>;

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

private:
    std::array<Element, CONSTANT_POOL_SIZE> data_;
};

class AsmEncoder {
public:
    static int Process(FILE *file);

    template<size_t opc_size, size_t op1_size, size_t op2_size>
    static void Encode(uint8_t opcode, uint8_t op1, uint8_t op2) {
        ASSERT((opc_size + op1_size + op2_size) == 16U);
        uint8_t operands = op2;
        operands <<= InstDecoder::SECOND_NEAR_REG_SHIFT;
        ASSERT(op1 <= InstDecoder::FIRST_NEAR_REG_MASK);
        operands |= op1;
        ENCODER.instructions_buffer_.emplace_back(opcode, operands);
    }

    template<size_t opc_size, size_t op1_size>
    static void Encode(uint8_t opcode, uint8_t operand) {
        ASSERT((opc_size + op1_size) == 16U);
        ENCODER.instructions_buffer_.emplace_back(opcode, operand);
    }
    template<size_t opc_size>
    static void Encode(uint8_t opcode) {
        uint8_t padding = 0;
        ASSERT((opc_size + sizeof(padding) * 8U) == 16U);
        ENCODER.instructions_buffer_.emplace_back(opcode, padding);
    }

    static auto &GetInstructionsBuffer()
    {
        return ENCODER.instructions_buffer_;
    }

    static void DeclareAndDefineFunction(char *c_str)
    {
        std::string key(c_str);
        ASSERT(ENCODER.declared_objects_.find(key) == ENCODER.declared_objects_.end());
        uint8_t idx = ENCODER.declared_objects_.size();
        ENCODER.declared_objects_[key] = idx;
        auto bc_offset = ENCODER.instructions_buffer_.size();
        ENCODER.constant_pool_.SetFunction(idx, bc_offset);
    }

    static void DeclareNum(char *c_str)
    {
        std::string key(c_str);
        ASSERT(ENCODER.declared_objects_.find(key) == ENCODER.declared_objects_.end());
        uint8_t idx = ENCODER.declared_objects_.size();
        ENCODER.declared_objects_[key] = idx;

        // NB: cache this idx because it is unhandy to resolve it later because num
        // declaration and definition is separated (see `assembler/templates/grammar.l.erb`):
        ENCODER.temp_idx_ = idx;
    }

    static void DefineNum(char *c_str)
    {
        double num = atof(c_str);
        ENCODER.constant_pool_.SetNum(ENCODER.temp_idx_, num);
    }

    static uint8_t ResolveName(const char *c_str)
    {
        std::string key(c_str);
        ASSERT(ENCODER.declared_objects_.find(key) != ENCODER.declared_objects_.end());
        return ENCODER.declared_objects_[key];
    }

    static ConstantPool &GetConstantPool()
    {
        return ENCODER.constant_pool_;
    }

private:
    Vector<BytecodeInstruction> instructions_buffer_ {};
    Hash<std::string, uint8_t> declared_objects_ {};
    ConstantPool constant_pool_ {};

    uint8_t temp_idx_ {};
};

} // namespace k3s

#endif  // ASSEMBLER_H
