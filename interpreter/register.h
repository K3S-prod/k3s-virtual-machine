#ifndef INTERPRETER_REGISTER
#define INTERPRETER_REGISTER

#include <cstdint>
#include "common/macro.h"

namespace k3s {

namespace coretypes {
class Array;
class Object;
class Function;
class String;
}

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
    void Set(double val) {
        type_ = Type::NUM;
        value_ = bit_cast<uint64_t>(val);
    }
    void Set(coretypes::String *val) {
        type_ = Type::STR;
        value_ = reinterpret_cast<uint64_t>(val);
    }
    void Set(coretypes::Function *val) {
        type_ = Type::FUNC;
        value_ = reinterpret_cast<uint64_t>(val);
    }
    void Set(coretypes::Object *val) {
        type_ = Type::OBJ;
        value_ = reinterpret_cast<uint64_t>(val);
    }
    void Set(coretypes::Array *array) 
    {
        type_ = Type::ARR;
        value_ = bit_cast<uint64_t>(array);
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
        case Type::STR:
            // TODO: implement this properly in .cpp file
            std::cout << "val_: " << reinterpret_cast<const char *>(value_ + 8) << "}" << std::endl;
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
        std::cout << "Reg[" << reg_id << "] : { type_: " << TypeToStr() << ", ";
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

    coretypes::Function *GetAsFunction() const {
        if (type_ != Type::FUNC) {
            LOG_FATAL(INTERPERTER, "TypeError: expected func");
        }
        return bit_cast<coretypes::Function *>(value_);
    }

    coretypes::Array *GetAsArray() const {
        if (type_ != Type::ARR) {
            LOG_FATAL(INTERPERTER, "TypeError: expected array");
        }
        return bit_cast<coretypes::Array *>(value_);
    }
    coretypes::Object *GetAsObject() const {
        if (type_ != Type::OBJ) {
            LOG_FATAL(INTERPERTER, "TypeError: expected object");
        }
        return bit_cast<coretypes::Object *>(value_);
    }
    coretypes::String *GetAsString() const {
        if (type_ != Type::STR) {
            LOG_FATAL(INTERPERTER, "TypeError: expected string");
        }
        return bit_cast<coretypes::String *>(value_);
    }

private:
    Type type_ {};
    uint64_t value_ {};
};

}

#endif  // INTERPRETER_REGISTER
