#ifndef ALLOCATOR_CONTAINERS_H
#define ALLOCATOR_CONTAINERS_H

#include "allocator.h"
#include <vector>
#include <array>
namespace k3s {

template <typename T>
using ConstVector = std::vector<T, decltype(Allocator().ConstRegion().Adapter<T>())>;
template <typename T>
using ArenaVector = std::vector<T, decltype(Allocator().RuntimeRegion().Adapter<T>())>;

}

#endif  // ALLOCATOR_CONTAINERS_H
