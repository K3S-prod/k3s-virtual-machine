#ifndef INTERPRETER_REGISTER
#define INTERPRETER_REGISTER

#include <cstdint>

namespace k3s {

class Register {
public:
#include "generated/reg_types.inl"
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

    void StoreValue(const uint64_t value)
    {
        value_ = value;
    }

private:
    Type type_ {};
    uint64_t value_ {};
};

}

#endif  // INTERPRETER_REGISTER