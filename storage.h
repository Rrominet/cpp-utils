#pragma once

#include "./Ret.h"
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

//the functions of write and read are async by default !
//You have a _sync version if you want.

//read is sync, because It's often called at the begining of the program and is quite fast.
namespace storage
{
    void init();
    void save(const std::function<void(size_t)>& cb = 0, const std::function<void(const std::string&)>& error = 0);

    ml::Ret<size_t> save_sync();

    json& data();

    template<typename T>
        T get(const std::string& key)
        {
            if (!data().contains(key))
                return T();
            return data()[key].get<T>(); 
        }

    template<typename T>
        void set(const std::string& key, const T& value, const std::function<void(size_t)>& cb = 0, const std::function<void(const std::string&)>& error = 0)
        {
            data()[key] = value;
            save(cb, error);
        }

    template<typename T>
        ml::Ret<size_t> set_sync(const std::string& key, const T& value)
        {
            data()[key] = value;
            return save_sync();
        }
}
