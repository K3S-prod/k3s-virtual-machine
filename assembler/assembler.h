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

    const auto &Elements() 
    {
        return data_;
    }

private:
    std::array<Element, CONSTANT_POOL_SIZE> data_;
};

class AsmEncoder {
public:
    static int Process(FILE *file);

    static void DumpToFile(FILE *file);

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

    static uint8_t TryResolveName(const char *c_str)
    {
        std::string key(c_str);
        if (ENCODER.declared_objects_.find(key) != ENCODER.declared_objects_.end()) {
            return ENCODER.declared_objects_[key];
        }
        
        // In fact, the inst isn't existing for now:
        size_t inst_with_pending_label_idx = ENCODER.instructions_buffer_.size();
        
        if (ENCODER.declared_labels_.find(key) != ENCODER.declared_labels_.end()) {
            size_t label_offset = ENCODER.declared_labels_[key];
            return bit_cast<uint8_t>(GetResolveRelativeOffset(label_offset, inst_with_pending_label_idx));
        }

        ENCODER.AppendUnresolvedLabels(key, inst_with_pending_label_idx);

        return 0;
    }
    
    static int8_t GetResolveRelativeOffset(size_t label_offset, size_t inst_offset)
    {
        auto signed_label_offset = static_cast<ptrdiff_t>(label_offset);
        auto signed_inst_offset = static_cast<ptrdiff_t>(inst_offset);
        auto res = static_cast<int8_t>(signed_label_offset - signed_inst_offset);
        return res;
    }

    void AppendUnresolvedLabels(const std::string &label_identifier, size_t inst_with_unresolved_label_idx)
    {
        unresolved_labels_[label_identifier].push_back(inst_with_unresolved_label_idx);
    }
    
    static void DefineLabel(const char *label_identifier_c_str)
    {
        std::string label_identifier(label_identifier_c_str);
        size_t label_offset = ENCODER.instructions_buffer_.size();
        if (ENCODER.declared_labels_.find(label_identifier) != ENCODER.declared_labels_.end()) {
            LOG_FATAL(ENCODER, "Label '" << label_identifier << "' redeclaration (inst_buffer_offset = " << ENCODER.declared_labels_[label_identifier] << ")");
        }
        
        ENCODER.declared_labels_[label_identifier] = label_offset;
        
        ENCODER.ResolveUnresolved(label_identifier);
    }

    void ResolveUnresolved(const std::string &label_identifier)
    {
        ASSERT(declared_labels_.find(label_identifier) != declared_labels_.end());
        size_t label_offset = declared_labels_[label_identifier];
    
        if (unresolved_labels_.find(label_identifier) != unresolved_labels_.end()) {
            for (auto inst_idx : unresolved_labels_[label_identifier]) {
                int8_t resolved_relative_offset = GetResolveRelativeOffset(label_offset, inst_idx);
                instructions_buffer_[inst_idx].SetOperands(bit_cast<uint8_t>(resolved_relative_offset));
            }
        }

        unresolved_labels_.erase(label_identifier);
    }

    static void CheckLabelsResolved()
    {
        if (ENCODER.unresolved_labels_.size() != 0) {
            LOG_FATAL(ENCODER, "There are " << ENCODER.unresolved_labels_.size() << " unresolved labels!");
        }
    }

    static ConstantPool &GetConstantPool()
    {
        return ENCODER.constant_pool_;
    }

private:
    Vector<BytecodeInstruction> instructions_buffer_ {};
    Hash<std::string, uint8_t> declared_objects_ {};
    Hash<std::string, size_t> declared_labels_ {};
    Hash<std::string, Vector<size_t>> unresolved_labels_ {};
    ConstantPool constant_pool_ {};

    uint8_t temp_idx_ {};
};

} // namespace k3s

#endif  // ASSEMBLER_H
