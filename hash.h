#pragma once
#include <string>

extern "C"
{
#include <sha3.h>
}

namespace hash
{
    std::string sha3(const std::string &data, const unsigned int &version=256);
}
