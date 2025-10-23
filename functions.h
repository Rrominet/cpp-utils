#pragma once
#include <string>

namespace functions
{
    template<typename T, typename F>
        void call(T* obj, F* funcPtr) 
        {
            (obj->*funcPtr)();
        }

    template<typename F>
        void call(F* funcPtr)
        {
            funcPtr();
        }

    template<typename T, typename F, typename A>
        void call(T* obj, F* funcPtr, A arg) 
        {
            (obj->*funcPtr)(arg);
        }
}
