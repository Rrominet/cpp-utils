#pragma once
#include "./Command.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ml
{
    template <typename T=json>
    class StateCommand : public Command
    {
        public : 
            StateCommand() : Command(){}
            virtual ~StateCommand() = default;

            virtual void exec() override 
            {
                if (!this->check())
                    throw std::runtime_error("Command check failed : " + this->name() + " : " + _error);

                if (!_getState)
                    throw std::runtime_error("No getState function set for this StateCommand : " + _name);
                
                _state = _getState();
                if (_exec)
                    _exec(_args);
            }

            virtual void reverse() override
            {
                if (!this->checkReverse())
                    throw std::runtime_error("Command check failed : " + this->name() + " : " + _error);

                if (!_setFromState)
                    throw std::runtime_error("No setFromState function set for this StateCommand : " + _name + " - (so can't reverse it.)");
                
                _setFromState(_state);
            }

        protected : 
            T _state;
            std::function<T()> _getState = 0;
            std::function<void(const T&)> _setFromState = 0;
    };
}
