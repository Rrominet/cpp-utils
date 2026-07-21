#include "./tconv.h"

namespace tconv
{
    void setError(
        std::string* error,
        std::string_view message)
    {
        if (error != nullptr)
            error->assign(message.data(), message.size());
    }
}
