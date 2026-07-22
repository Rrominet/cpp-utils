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

    bool parseBool(
            std::string_view text,
            bool& result,
            std::string* error)
    {
        text = str::trim(text);

        if (text == "1" || str::equalsIgnoreCase(text, "true"))
        {
            result = true;
            return true;
        }

        if (text == "0" || str::equalsIgnoreCase(text, "false"))
        {
            result = false;
            return true;
        }

        setError(
            error,
            "Invalid boolean string. Expected true, false, 1, or 0.");

        result = false;
        return false;
    }
}
