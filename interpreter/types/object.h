#ifndef INTERPRETER_TYPES_OBJECT_H
#define INTERPRETER_TYPES_OBJECT_H

#include "allocator/containers.h"
#include "function.h"
#include <string_view>

namespace k3s::coretypes {

class Object {
public:
    using MappingT = ConstUnorderedMap<std::string_view, size_t>;

    template <uintptr_t START_PTR, size_t SIZE>
    Object( Allocator::Region<START_PTR, SIZE> region, const MappingT &map, size_t n_methods,
            const size_t *bc_offsets) : map_(map)
    {
        size_t total_members = map.size();
        ASSERT(total_members >= n_methods);
        for (size_t i = 0; i < n_methods; i++) {
            fields_[total_members - n_methods + i].Set(Function::New(region, bc_offsets[i]));
        }
    }

    void SetElem(const char *id, const Register &val)
    {
        fields_[ResolveId(id)] = val;
    }
    Register &GetElem(const char *id)
    { 
        return fields_[ResolveId(id)]; 
    }

    template <uintptr_t START_PTR, size_t SIZE>
    static Object *New( Allocator::Region<START_PTR, SIZE> region,
                        const coretypes::Object::MappingT &mapping,
                        const size_t *bc_offsets_vector);
private:
    size_t ResolveId(const char *id)
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

template <uintptr_t START_PTR, size_t SIZE>
inline Object *Object::New( Allocator::Region<START_PTR, SIZE> region,
                            const coretypes::Object::MappingT &mapping,
                            const size_t *bc_offsets_vector)
{
    size_t methods_n = bc_offsets_vector[0];
    void *ptr = region.AllocBytes(sizeof(coretypes::Object) + sizeof(Register) * mapping.size());
    return new (ptr) coretypes::Object(region, mapping, methods_n, bc_offsets_vector + 1);
}

}

#endif  // INTERPRETER_TYPES_OBJECT_H
