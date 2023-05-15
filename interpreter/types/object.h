#ifndef INTERPRETER_TYPES_OBJECT_H
#define INTERPRETER_TYPES_OBJECT_H

#include "allocator/containers.h"
#include "allocator/object_header.h"
#include "function.h"
#include <string_view>

namespace k3s::coretypes {

class Object : public ObjectHeader
{
public:
    using MappingT = ConstUnorderedMap<std::string_view, size_t>;

    template <uintptr_t START_PTR, size_t SIZE>
    Object( GCRegion<START_PTR, SIZE> region, const MappingT &map, size_t n_methods,
            const size_t *bc_offsets) : map_(map)
    {
        size_t total_members = map.size();
        ASSERT(total_members >= n_methods);
        for (size_t i = 0; i < total_members - n_methods; i++) {
            fields_[i].Reset();
        }
        for (size_t i = 0; i < n_methods; i++) {
            fields_[total_members - n_methods + i].Set(Function::New(region, bc_offsets[i]));
        }
    }

    void SetElem(const char *id, const Register &val)
    {
        fields_[ResolveId(id)] = val;
    }
    Register *GetElem(const char *id)
    { 
        return &fields_[ResolveId(id)]; 
    }
    Register *GetElem(size_t idx)
    { 
        return &fields_[idx]; 
    }
    size_t GetSize() const
    {
        return map_.size();
    }

    template <uintptr_t START_PTR, size_t SIZE>
    static Object *New( GCRegion<START_PTR, SIZE> region,
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
inline Object *Object::New( GCRegion<START_PTR, SIZE> region,
                            const coretypes::Object::MappingT &mapping,
                            const size_t *bc_offsets_vector)
{
    size_t obj_allocated_size = sizeof(coretypes::Object) + sizeof(Register) * mapping.size();
    size_t methods_n = bc_offsets_vector[0];
    size_t methods_allocated_size = sizeof(coretypes::Function) * methods_n;
    region.PrepareForSequentAllocations(methods_allocated_size + obj_allocated_size);

    void *storage = region.AllocBytes(obj_allocated_size);
    auto *ptr = new (storage) coretypes::Object(region, mapping, methods_n, bc_offsets_vector + 1);
    ptr->SetAllocatedSize(obj_allocated_size);

    region.EndSequentAllocations();
    return ptr;
}

}

#endif  // INTERPRETER_TYPES_OBJECT_H
