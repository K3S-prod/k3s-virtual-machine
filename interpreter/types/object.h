#ifndef INTERPRETER_TYPES_OBJECT_H
#define INTERPRETER_TYPES_OBJECT_H

#include "allocator/containers.h"
#include <string_view>

namespace k3s::coretypes {

class Object {
public:
    using MappingT = ConstUnorderedMap<std::string_view, size_t>;

    Object(const MappingT &map) : map_(map)
    {}

    void SetElem(std::string_view id, const Register &val)
    {
        fields_[ResolveId(id)] = val;
    }
    Register &GetElem(std::string_view id)
    { 
        return fields_[ResolveId(id)]; 
    }

private:
    size_t ResolveId(std::string_view id)
    {
        if (map_.find(id) == map_.end()) {
            LOG_FATAL(INTERPRETER, "Object is missing field: '" << id << "'");
        }
        return map_.find(id)->second;
    }

private:
    const MappingT &map_;
    Register fields_[]; 
};

}

#endif  // INTERPRETER_TYPES_OBJECT_H
