#ifndef ALLOCATOR_CONTAINERS_H
#define ALLOCATOR_CONTAINERS_H

#include "allocator.h"
#include <vector>
#include <array>
#include <string>
#include <unordered_map>

namespace k3s {

template <typename T>
using ConstVector = std::vector<T, decltype(Allocator().ConstRegion().Adapter<T>())>;
template <typename T>
using ArenaVector = std::vector<T, decltype(Allocator().ObjectsRegion().Adapter<T>())>;

template <typename T>
using StackVector = std::vector<T, decltype(Allocator().StackRegion().Adapter<T>())>;

template <typename T>
using GCVector = std::vector<T, decltype(Allocator().GcInternalsRegion().Adapter<T>())>;

template <typename Key, typename T>
using ConstUnorderedMap = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, decltype(Allocator().ConstRegion().Adapter<std::pair<const Key, T>>())>;

using ConstString = std::basic_string<char, std::char_traits<char>, decltype(Allocator().ConstRegion().Adapter<char>())>;

}

#endif  // ALLOCATOR_CONTAINERS_H
