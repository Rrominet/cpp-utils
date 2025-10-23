#pragma once
#include "vec.h"
namespace ml
{
    //TODO : test it !
    template<typename T>
        class HistoryStack
        {
            public : 

                HistoryStack(const T& initialState=T(), unsigned long max=100000) : _max(max)
            {
                this->store(initialState);
            }
                ~HistoryStack(){}

                void store(const T& data)
                {
                    if (_current + 1 >= _stack.size())
                        _stack.push_back(data);
                    else 
                    {
                        _stack[_current + 1] = data;
                        _stack.resize(_current + 2);
                    }
                    _current = _stack.size() - 1;
                }

                T& undo()
                {
                    if (_current<0)
                        throw std::length_error("No more undo possible.");
                    _current -- ;
                    return _stack[_current];
                }

                T& redo()
                {
                    if (_current >= _stack.size())
                        throw std::length_error("No more redo possible.");

                    _current ++;
                    return _stack[_current];
                }

                const T& current() const
                {
                    if (_current < 0 || _current >= _stack.size())
                        throw std::length_error("No current. Current : " + std::to_string(_current));
                    return _stack[_current];
                }

                unsigned long max() const {return _max;}
                void setMax(unsigned long max) {_max = max;}

            private : 
                ml::Vec<T> _stack;
                int _current = -1;
                unsigned long _max;
        };
}
