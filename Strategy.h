#pragma once
#include <any>

// this pattern is really similar to the command pattern
// but its way more simple and is not designed for the same stuff
// Command should be used with a command manager and are more flexible because they don't need to override their methods to changes their behavior (that can just call setExec, ...)
// They are design to manage complex undo and redo stuff, etc...
//
// Strategy on the other hand is just a function that return any and that can be reimplement very easily 
//
// Case of use : 
// use it when you want to be able to change your algorithms/system at runtime
//
// Example : 
// you have 2 Strategy reimplement : 
// SortByDate and SortByName
//
// so you can have this 
// class App
// {
//    List _list;
//    Strategy _sort = SortByName();
//
//    void sort()
//    {
//        _list = std::any_cast<List>(_sort(_list));
//    }
//
//    // here you can customize easily the sorting behavior only doing this : 
//    void setSort(Strategy s)
//    {
//        _sort = s;
//    }
//    // you could even let the _sort member to be public
//    // and do app._sort = SortByDate();
// }
//
// Hope it's clear.

namespace ml
{
    class Strategy
    {
        public : 
            virtual std::any operator()(const std::any& args)=0;
    };
}
