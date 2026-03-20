#pragma once
#include <any>
#include <string>
#include <unordered_map>
#include "./Ret.h"

namespace ml
{
    class AnyData
    {
        public : 
            template <typename T>
                void add(const T& t, const std::string& key)
                {
                    _data[key] = std::any(t); 
                }

            //this return a copy, use get_ptr if you want a pointer/reference
            template <typename T>
                ml::Ret<T> get(const std::string& key) const
                {
                    if (_data.find(key) != _data.end())
                    {
                        try 
                        {
                            return ml::ret::success<T>(std::any_cast<T>(_data.at(key)));
                        }
                        catch(const std::exception& e)
                        {
                            return ml::ret::fail<T>("The type conversion failed in this AnyData instance for the key " + key + ".\nType given :  " + typeid(T).name() + "\nType stored : " + typeid(_data.at(key)).name());
                        }
                    }
                    return ml::ret::fail<T>("The key " + key + " does not exists in this AnyData instance.");
                }

            template <typename T>
                ml::Ret<T*> get_ptr(const std::string& key) const
                {
                    auto r = this->get<T>(key);
                    if (r.success)
                        return ml::ret::success<T*>(&r.value);
                    return ml::ret::fail<T*>(r.error);
                }

            bool has(const std::string& data) const
            {
                return _data.find(data) != _data.end();
            }

        private : 
            std::unordered_map<std::string, std::any> _data;
    };
}
