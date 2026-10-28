#pragma once
#include <stdexcept>
#include <string>
#include <map>

namespace ml
{
    class BaseArgv
    {
        public : 
            BaseArgv(const std::map<std::string, std::string>& map={}) : _argv(map){}
            virtual ~BaseArgv() = default;

            bool has(const std::string& key);

            const std::map<std::string, std::string>& argv()const{return _argv;}
            bool empty() const {return _argv.empty();}
            size_t size() const noexcept {return _argv.size();}

            std::map<std::string, std::string>::const_iterator find(const std::string& key) const{return _argv.find(key);}

            const std::string& at(const std::string& key) const
            {
                if (_argv.find(key) == _argv.end())
                    throw std::out_of_range("key " + key + " not found");
                return _argv.at(key);
            }
            const std::string& operator[](const std::string& key) const {
                return this->at(key);
            }

            std::map<std::string, std::string>::const_iterator end() const {
                return _argv.end();
            }

        protected : 
            std::map<std::string, std::string> _argv;
    };
}

