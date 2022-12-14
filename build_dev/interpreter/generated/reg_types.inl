enum class Type : uint8_t {
    // Type to represent real and integer values.
    NUM,
    // Type to represent arrays of elements of any types.
    ARR,
    // Type to represent strings.
    STR,
    // Type to represent objects with named fields. Each field must have a type and object's layout can't be modified at runtime. Store reg to field of different type is a run-time error.
    OBJ,
    // Type to represent functions as objects. Registers of such type can be "invoked".
    FUNC,
    // Empty/undefined value.
    NIL,
    // Unspecified type.
    ANY,

};

std::string TypeToStr() {
    switch(type_) {
        case Type::NUM : {
            return std::string("NUM");
        }
        case Type::ARR : {
            return std::string("ARR");
        }
        case Type::STR : {
            return std::string("STR");
        }
        case Type::OBJ : {
            return std::string("OBJ");
        }
        case Type::FUNC : {
            return std::string("FUNC");
        }
        case Type::NIL : {
            return std::string("NIL");
        }
        case Type::ANY : {
            return std::string("ANY");
        }

        default: {
            LOG_FATAL(INTERPRETER, "Unknown type in Dump()");
        }
    }
}
