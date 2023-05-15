#ifndef INTERPRETER_REGISTER
#define INTERPRETER_REGISTER

#include <cstdint>
#include "allocator/object_header.h"
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
    Register(coretypes::Function *func)
    {
        Set(func);
    }
    Register(Type type, uint64_t value): type_(type), value_(value) {}

    bool IsPrimitive() const
    {
        return (GetType() == Type::NUM) || (GetType() == Type::ANY);
    }
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
    void Reset() {
        type_ = Type::ANY;
        value_ = 0;
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

    void Dump(size_t recursion_level = 0);

    double GetAsNum() const {
        return bit_cast<double>(value_);
    }
    
    auto *GetAsObjectHeader() const
    {
        ASSERT(!IsPrimitive());
        return reinterpret_cast<ObjectHeader *>(value_);
    }
    auto *GetObjectHeaderPtr()
    {
        ASSERT(!IsPrimitive());
        return reinterpret_cast<ObjectHeader **>(&value_);
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
    Type type_ {Type::ANY};
    uint64_t value_ {0};
};

}

#endif  // INTERPRETER_REGISTER
