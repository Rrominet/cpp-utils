#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "templates.h"
#include "vec.h"

namespace ml::json
{
    //objects map mean that the second template paramter of your map need to be an object instance that can execute a execute method
    template<typename O, template<typename, typename> class Map>
        nlohmann::json serializeObjectsMap(const Map<std::string, O>& data)
        {
            nlohmann::json _r;  
            for (const auto& [key, val] : data )
                _r[key] = tpl::instance(val).serialize(); // TODO : need to test with actual data
            return _r;
        }

    //work only with basic types of c++
    template<typename T, template<typename, typename> class Map>
        nlohmann::json serializeSimpleMap(const Map<std::string, T>& data)
        {
            nlohmann::json _r;  
            for (const auto& [key, val] : data )
                _r[key] = val;
            return _r;
        }

    //work only with basic types of c++
    template<typename T, template<typename, typename> class Map>
        Map<std::string, T> deserializeSimpleMap(const nlohmann::json& data)
        {
            Map<std::string, T> _r;
            for (const auto& [key, val] : data.items())
                _r[key] = (T)val;
            return _r;
        }
    //
    //objects map mean that the second template paramter of your map need to be an object instance that can execute a execute method
    template<typename O, template<typename> class List>
        nlohmann::json serializeList(const List<O>& data)
        {
            nlohmann::json _r = {};  
            for (const auto& o : data)
                _r.push_back(tpl::instance(o).serialize());
            return _r;
        }

    // the content of the list have to be of type 'simple' like int/bool/string/ etc... type that json understand natively
    template<typename O, template<typename> class List>
        nlohmann::json serializeSimpleList(const List<O>& data)
        {
            nlohmann::json _r = nlohmann::json::array();  
            for (const auto& o : data)
                _r.push_back(tpl::instance(o));
            return _r;
        }

    nlohmann::json updated(const nlohmann::json& old, const nlohmann::json& _new);
    nlohmann::json merged(const nlohmann::json& a, const nlohmann::json& b);
    void merge(nlohmann::json& src, const nlohmann::json& with);


    //return true if a and b are equal (meaning their key and values match)
    bool compare(const nlohmann::json& a, const nlohmann::json& b);

    template<typename F>
        // F signature : void (const std::sttring& key, const json& value)
        void doOnEachAttr(nlohmann::json& data, F f)
        {
            for (auto& el : data.items())
            {
                f(el.key(), el.value());
                if (el.value().is_structured())
                    doOnEachAttr(el.value(), f);
            }
        }
    
    template<typename T=std::string, template<typename> class List=ml::Vec>
        List<T> asVector(const nlohmann::json& data)
        {
            List<T> _r;
            for(const auto& d : data)
                _r.push_back(d.get<T>());
            return _r;
        }

    template<typename T=std::string, typename U=std::string, template<typename, typename> typename Container=std::unordered_map>
        nlohmann::json mapAsJson(const Container<T, U>& data)
        {
            nlohmann::json _r;
            for (const auto& d : data)
                _r[d.first] = d.second;

            return _r;
        }
}
