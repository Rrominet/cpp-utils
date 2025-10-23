#pragma once
#include <string>

namespace uri
{
    std::string decode(std::string data);
    std::string encode(std::string data);
    std::string test();
    void _setRefs();
}
