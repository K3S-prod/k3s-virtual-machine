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

    void Set(Type type, uint64_t val) {
        type_ = type;
        value_ = val;
    }
    
    void Set(const Register &reg) {
        type_ = reg.type_;
        value_ = reg.value_;
    }

    void Dump() {
        std::cout << "Reg type: " << static_cast<int>(type_) << std::endl 
                  << "Reg value: " << value_ << std::endl;
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
