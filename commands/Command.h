#pragma once
#include <string>
#include <functional>
#include <stdexcept>
#include <any>
#include "./vec.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ml
{
    class Command    
    {
        public : 
            Command() = default;
            virtual ~Command() = default;

            virtual void exec();
            virtual void reverse();

            virtual void setExec(const std::function<void(const std::any&)>& f) { _exec = f; }
            void setReverse(const std::function<void(const std::any&)>& f) { _reverse = f; }

            void setArgs(const std::any& args) {_args = args;}

            std::any& argsAsAny(){return _args;}

            template<typename T>
                T& args()
                {
                    if (!_args.has_value())
                        throw std::runtime_error("No args set for this command : " + _name);
                    return std::any_cast<T&>(_args);
                }

            // if this return false, the cammand will throw an exception when called
            virtual bool check(){return true;}
            virtual bool checkReverse(){return true;}

            virtual json serialize() const;
            virtual void deserialize(const json& j);

        protected : 
            std::function<void(const std::any&)> _exec = 0;
            std::function<void(const std::any&)> _reverse = 0;

            std::any _args;
            std::string _id; //bp cgs
            std::string _help; //bp cgs
            std::string _name; //bp cgs
            std::string _error; //bp cgs
            std::string _keybind; //bp cgs
            ml::Vec <std::string> _aliases; //bp cg

            // will be save in the json serialized
            json _userData; //bp cgs

            // this value can be setted in the _exec function to return a value
            // if you doing this, you will need to keep track of the command reference/ptr after its execution to get the returned value with command.returned()
            std::any _returned; //bp cgs
                              
        public : 
#include "./Command_gen.h"
    };
}
