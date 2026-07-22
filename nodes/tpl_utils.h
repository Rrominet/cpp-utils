#pragma once
#include <type_traits>

template<typename T, typename=void>
struct has_id : std::false_type {};

template<typename T>
struct has_id<T, std::void_t<decltype(std::declval<T>().id())>> : std::true_type {};

// usage : 
// if constexpr(has_id<YourType>::value)
