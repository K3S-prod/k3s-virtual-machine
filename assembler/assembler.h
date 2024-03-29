#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "interpreter/generated/inst_decoder.h"
#include "interpreter/types/coretypes.h"
#include "common/macro.h"
#include "classfile/class_file.h"
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

class AsmEncoder {
    struct ObjectDescr
    {
        Vector<std::string> data_fields_;
        Vector<std::string> methods_;
        Vector<size_t> methods_bc_offsets_;
    };
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
        DeclareId(c_str);
        auto bc_offset = ENCODER.instructions_buffer_.size();
        LOG_DEBUG(ASSEMBLER, "Function `" << c_str << "` (pc " << bc_offset << ")");
        ENCODER.constant_pool_.SetFunction(ENCODER.temp_idx_, bc_offset);
    }
    
    static void DeclareAndDefineMethod(char *c_str)
    {
        auto bc_offset = ENCODER.instructions_buffer_.size();
        LOG_DEBUG(ASSEMBLER, "Method `" << c_str << "` (pc " << bc_offset << ")");
        ENCODER.objects_storage_.back().methods_bc_offsets_.push_back(bc_offset);
        ENCODER.objects_storage_.back().methods_.emplace_back(c_str);
    }
    
    static void DeclareAndDefineAnyDataMember(char *c_str)
    {
        ENCODER.objects_storage_.back().data_fields_.emplace_back(c_str);
    }

    static void DeclareObject(char *c_str)
    {
        DeclareId(c_str);
        ENCODER.constant_pool_.SetObject(ENCODER.temp_idx_, ENCODER.objects_storage_.size());
        ENCODER.objects_storage_.emplace_back();
    }

    static void FinalizeObject()
    {
        ENCODER.is_class_context_ = false;
    }

    static void DeclareId(char *c_str)
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

    static void DefineStr(const char *c_str)
    {
        ASSERT(c_str[0] == '"');
        ENCODER.strings_storage_.emplace_back(c_str + 1);
        ASSERT(ENCODER.strings_storage_.back().back() == '"');
        ENCODER.strings_storage_.back().pop_back();
        ENCODER.constant_pool_.SetStr(ENCODER.temp_idx_, ENCODER.strings_storage_.size() - 1);
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
        if (ENCODER.instructions_buffer_.back().GetOpcode() != Opcode::RET) {
            LOG_FATAL(ENCODER, "Function should end with `RET`");
        } 
    }

    static ConstantPool &GetConstantPool()
    {
        return ENCODER.constant_pool_;
    }

    const auto &GetStringsStorage()
    {
        return strings_storage_;
    }
    
    const auto &GetObjectsStorage()
    {
        return objects_storage_;
    }

private:
    Vector<BytecodeInstruction> instructions_buffer_ {};
    Hash<std::string, uint8_t> declared_objects_ {};
    Hash<std::string, size_t> declared_labels_ {};
    Hash<std::string, Vector<size_t>> unresolved_labels_ {};
    Vector<std::string> strings_storage_ {};
    Vector<ObjectDescr> objects_storage_ {};
    ConstantPool constant_pool_ {};

    uint8_t temp_idx_ {};
    bool is_class_context_ {false};
};

} // namespace k3s

#endif  // ASSEMBLER_H
