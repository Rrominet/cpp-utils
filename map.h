#pragma once
#include <unordered_map>

namespace ml
{
    namespace map
    {
        template<typename K, typename V>
        bool contains(const std::unordered_map<K, V>& map, const K& key)
            { return map.find(key) != map.end(); }
    }
}
