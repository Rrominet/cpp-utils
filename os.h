#pragma once
#include <string>
#include <vector>
#include <map>
#include "thread.h"
namespace os
{
    std::string env(const std::string& envname);
    std::string type();
    std::vector<std::string> appPaths();
    std::vector<std::string> iconPaths();
    std::string icon_filepath(const std::string& name);

    std::string home();
    std::string clipboard();
    void setClipboard(std::string text);

#ifdef __linux__
    namespace lin
    {
        std::string home();
    }
#elif _WIN32
    namespace win
    {
        std::string home();
    }
#elif __APPLE__
    namespace apple
    {

    }
#endif
}
