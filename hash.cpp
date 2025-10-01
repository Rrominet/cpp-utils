#include "hash.h"
#include "debug.h"
#include <iomanip>
#include <iostream>

std::string hash::sha3(const std::string &data, const unsigned int &version)
{
    uint8_t out[version/8];
    const void* cdata = data.c_str();
    sha3_HashBuffer(version, SHA3_FLAGS_KECCAK, cdata, data.length(), out, sizeof(out));

    //lg2("Hashed as int 8", out);
    std::stringstream ss;
    for(int i=0; i<version/8; ++i)
        ss << std::hex << (int)out[i];
    return ss.str();
}
