#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace storage
{
    void init();
    void save();
    json& data();

    template<typename T>
        T get(const std::string& key)
        {
            if (!data().contains(key))
                return T();
            return data()[key].get<T>(); 
        }

    template<typename T>
        void set(const std::string& key, const T& value)
        {
            data()[key] = value;
            save();
        }
}
