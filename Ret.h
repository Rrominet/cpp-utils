#pragma once
#include <string>

namespace ml
{
    template<typename T=bool>
        class Ret
        {
            public : 
                T value;
                std::string message;
                bool success = false;
        };

    namespace ret
    {
        template <typename T=bool>
            Ret<T> success(T value, const std::string& message="")
            {
                Ret<T> ret;
                ret.value = value;
                ret.message = message;
                ret.success = true;
                return ret;
            }

            Ret<bool> success(const std::string& message="");

        template <typename T=bool>
            Ret<T> failure(const std::string& message="")
            {
                Ret<T> ret;
                ret.message = message;
                ret.success = false;
                return ret;
            }
        template <typename T=bool>
            Ret<T> fail(const std::string& message="")
            {
                return failure<T>(message);
            }
    }
}
