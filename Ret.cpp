#include "./Ret.h"

namespace ml
{
    namespace ret
    {
        Ret<bool> success(const std::string& message, int code)
        {
            Ret<bool> ret;
            ret.value = true;
            ret.message = message;
            ret.success = true;
            ret.code = code;
            return ret;
        }
        Ret<bool> ok(const std::string& message, int code)
        {
            return success(message, code);
        }
    }
}
