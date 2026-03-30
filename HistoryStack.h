#pragma once
#include "vec.h"
namespace ml
{
    template<typename T>
        class HistoryStack
        {
            public : 

                HistoryStack(unsigned long max=100000) : _max(max)
                {
                }
                ~HistoryStack(){}

                void store(const T& data)
                {
                    // Enforce max size before adding
                    if (_stack.size() >= _max && _max > 0)
                    {
                        _stack.vec.erase(_stack.begin());
                        if (_current > 0)
                            _current--;
                    }
                    
                    // If we're not at the end, remove everything after current
                    if (_current + 1 < static_cast<long>(_stack.size()))
                    {
                        _stack.resize(_current + 1);
                    }
                    
                    _stack.push_back(data);
                    _current = static_cast<long>(_stack.size()) - 1;
                }
                void push(const T& data){this->store(data);}
                void add(const T& data){this->store(data);}

                T& undo()
                {
                    lg("Data in stack : " << _stack.size());
                    if (_stack.size() == 0)
                        throw std::length_error("No data in stack for undo.");
                    if (_current <= 0)
                    {
                        lg("No more undo possible.");
                        return _stack[0];
                    }
                    _current--;
                    return _stack[_current];
                }

                T& redo()
                {
                    if (_stack.size() == 0)
                        throw std::length_error("No data in stack for redo.");
                    if (_current >= static_cast<long>(_stack.size()) - 1)
                    {
                        lg("No more redo possible.");
                        return _stack[_current];
                    }
                    
                    _current++;
                    return _stack[_current];
                }

                const T& current() const
                {
                    if (_current < 0 || _current >= static_cast<long>(_stack.size()))
                        throw std::length_error("No current. Current : " + std::to_string(_current));
                    return _stack[_current];
                }

                bool canUndo() const 
                {
                    return _current > 0;
                }

                bool canRedo() const 
                {
                    return _current < static_cast<long>(_stack.size()) - 1;
                }

                unsigned long max() const {return _max;}
                void setMax(unsigned long max) {_max = max;}

            private : 
                ml::Vec<T> _stack;
                long _current = -1;
                unsigned long _max;
        };
}
