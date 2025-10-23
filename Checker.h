#pragma once
#include <unordered_map>
#include <string>

namespace ml
{
    class Checker
    {
        public : 
            Checker() = default;
            ~Checker() = default;

            void init(const std::string& key, const std::string& description);
            void set(const std::string& key, bool value);
            bool check(const std::string& key, bool _assert=true);

        private : 
            std::unordered_map<std::string, bool> _map;
            std::unordered_map<std::string, std::string> _infos;

    };
}
