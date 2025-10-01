#include "./Ret.h"

namespace ml
{
    namespace ret
    {
        Ret<bool> success(const std::string& message)
        {
            Ret<bool> ret;
            ret.value = true;
            ret.message = message;
            ret.success = true;
            return ret;
        }
    }
}
