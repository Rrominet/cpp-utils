#pragma once
#include <string>
#include "./debug.h"

namespace ml
{
    template<typename T=bool>
        class Ret
        {
            public : 
                T value;
                std::string message;
                bool success = false;
                int code = 0;
        };

    namespace ret
    {
        template <typename T=bool>
            Ret<T> success(T value, const std::string& message="", int code=0)
            {
                Ret<T> ret;
                ret.value = value;
                ret.message = message;
                ret.success = true;
                ret.code = code;
                return ret;
            }

            Ret<bool> success(const std::string& message="", int code=0);

        template <typename T=bool>
            Ret<T> ok(T value, const std::string& message="", int code=0)
            {
                return success<T>(value, message, code);
            }

            Ret<bool> ok(const std::string& message="", int code=0);

        template <typename T=bool>
            Ret<T> failure(const std::string& message="", int code=0)
            {
                lg((std::string)"ml::Ret Failure : " + message); 
                Ret<T> ret;
                ret.message = message;
                ret.success = false;
                ret.code = code;
                return ret;
            }
        template <typename T=bool>
            Ret<T> fail(const std::string& message="", int code=0)
            {
                return failure<T>(message, code);
            }
    }
}
