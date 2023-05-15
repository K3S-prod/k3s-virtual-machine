#include "register.h"
#include "types/coretypes.h"

namespace k3s {

void DumpArray(coretypes::Array *arr, size_t recursion_level)
{
    std::cout << "size_: " << arr->GetSize() << "; data_:\n" ;
    std::string indent(recursion_level, '-');
    for (size_t i = 0; i < arr->GetSize(); i++) {
        std::cout << indent << " [" << i << "] ";
        arr->GetElem(i)->Dump(recursion_level + 1);
    }
    std::cout << indent << "}\n";
}

void DumpObject(coretypes::Object *obj, size_t recursion_level)
{
    std::cout << "size_: " << obj->GetSize() << "; data_:\n" ;
    std::string indent(recursion_level, '-');
    for (size_t i = 0; i < obj->GetSize(); i++) {
        if (obj->GetElem(i)->GetType() != Register::Type::FUNC) {
            std::cout << indent << " [" << i << "] ";
            obj->GetElem(i)->Dump(recursion_level + 1);
        }
    }
    std::cout << indent << "}\n";
}


void Register::Dump(size_t recursion_level)
{
    std::string indent(recursion_level, '-');

    std::cout << "{ type_: " << TypeToStr() << ", ";
    switch (type_) {
    case Type::NUM:
        std::cout << "val_: " << std::fixed << GetAsNum() << "}\n";
        break;
    case Type::STR:
        std::cout << "val_: " << GetAsString()->GetData() << "}\n";
        break;
    case Type::ARR:
        DumpArray(GetAsArray(), recursion_level);
        break;
    case Type::OBJ:
        DumpObject(GetAsObject(), recursion_level);
        break;
    case Type::FUNC:
        std::cout << "target_pc_: " << GetAsFunction()->GetTargetPc() << "}\n";
        break;
    default:
        std::cout << "val_: " << value_ << "}\n";
        break;
    }
}

}
