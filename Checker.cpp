#include "./Checker.h"
#include <cassert>
#include "debug.h"

namespace ml 
{
    void Checker::set(const std::string& key,bool value)
    {
        _map[key] = value;
    }

    void Checker::init(const std::string& key,const std::string& description)
    {
        _infos[key] = description;	
    }

    bool Checker::check(const std::string& key, bool _assert)
    {
        if (_map.find(key) == _map.end())
        {
            lg("key: " + key + " not found for assertion.");
            lg("Keys available : ");
            for (auto &k : _map)
                lg(k.first);
            assert(false);
        }
        bool res = _map[key];
        if (_assert && !res)
        {
            lg("key: " + key + " value: " + std::to_string(res));
            lg("It does not pass the checker. (Use GDB to get the line.)");
            if (_infos.find(key) != _infos.end())
                lg("More infos : " + _infos[key]);
            assert(res);
        }
        return res;
    }
}
