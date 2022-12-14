#ifndef INTERPRETER_REGISTER
#define INTERPRETER_REGISTER

#include <cstdint>
#include "common/macro.h"

namespace k3s {

class Register {
public:
#include "interpreter/generated/reg_types.inl"
    Register() = default;
    Register(Type type, uint64_t value): type_(type), value_(value) {}

    Type GetType() const
    {
        return type_;
    }

    uint64_t GetValue() const
    {
        return value_;
    }

    void Set(Type type, uint64_t val)
    {
        ASSERT(type != Type::NUM && "Num should be set via SetNum");
        type_ = type;
        value_ = val;
    }
    void SetNum(double val) {
        type_ = Type::NUM;
        value_ = bit_cast<uint64_t>(val);
    }
    
    void Set(const Register &reg) {
        type_ = reg.type_;
        value_ = reg.value_;
    }

    void DumpAcc(size_t recursion_level = 0)
    {
        for (size_t i = 0; i < recursion_level; i++) {
            std::cout << "-";
        }
        std::cout << "Acc : { type_: " << static_cast<int>(type_) << ", ";
        switch (type_) {
        case Type::NUM:
            std::cout << "val_: " << bit_cast<double>(value_) << "}" << std::endl;
            break;        
        default:
            std::cout << "val_: " << value_ << "}" << std::endl;
            break;
        }
    }

    void Dump(size_t reg_id, size_t recursion_level = 0)
    {
        for (size_t i = 0; i < recursion_level; i++) {
            std::cout << "-";
        }
        std::cout << "Reg[" << reg_id << "] : { type_:" << static_cast<int>(type_) << ", ";
        switch (type_) {
        case Type::NUM:
            std::cout << "val_: " << bit_cast<double>(value_) << "}" << std::endl;
            break;        
        default:
            std::cout << "val_: " << value_ << "}" << std::endl;
            break;
        }
    }

    double GetAsNum() const {
        return bit_cast<double>(value_);
    }

private:
    Type type_ {};
    uint64_t value_ {};
};

}

#endif  // INTERPRETER_REGISTER
