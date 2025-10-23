#include "base64.h"
#include "files.2/files.h"

using namespace base64;

std::string base64::encode(const std::vector<BYTE>& buf)
{
    if (buf.empty())
        return ""; // Avoid dereferencing buf if it's empty
    return encode(&buf[0], (unsigned int)buf.size());
}
std::string base64::encode(const BYTE* buf, unsigned int bufLen)
{
    size_t missing = 0;
    size_t ret_size = bufLen;
    while ((ret_size % 3) != 0)
    {
        ++ret_size;
        ++missing;
    }

    // Expand the return string size to a multiple of 4
    ret_size = 4*ret_size/3;

    std::string ret;
    ret.reserve(ret_size);

    for (unsigned int i=0; i<ret_size/4; ++i)
    {
        // Read a group of three bytes (avoid buffer overrun by replacing with 0)
        size_t index = i*3;
        BYTE b3[3];
        b3[0] = (index+0 < bufLen) ? buf[index+0] : 0;
        b3[1] = (index+1 < bufLen) ? buf[index+1] : 0;
        b3[2] = (index+2 < bufLen) ? buf[index+2] : 0;

        // Transform into four base 64 characters
        BYTE b4[4];
        b4[0] =                            ((b3[0] & 0xfc) >> 2);
        b4[1] = ((b3[0] & 0x03) << 4) +    ((b3[1] & 0xf0) >> 4);
        b4[2] = ((b3[1] & 0x0f) << 2) +    ((b3[2] & 0xc0) >> 6);
        b4[3] = ((b3[2] & 0x3f) << 0);

        // Add the base 64 characters to the return value
        ret.push_back(to_base64[b4[0]]);
        ret.push_back(to_base64[b4[1]]);
        ret.push_back(to_base64[b4[2]]);
        ret.push_back(to_base64[b4[3]]);
    }

    // Replace data that is invalid (always as many as there are missing bytes)
    for (size_t i=0; i<missing; ++i)
        ret[ret_size - i - 1] = '=';

    return ret;
}
std::vector<BYTE> base64::decode(std::string encoded_string)
{
    while ((encoded_string.size() % 4) != 0)
        encoded_string.push_back('=');

    size_t encoded_size = encoded_string.size();
    std::vector<BYTE> ret;
    ret.reserve(3*encoded_size/4);

    for (size_t i=0; i<encoded_size; i += 4)
    {
        // Get values for each group of four base 64 characters
        BYTE b4[4];
        b4[0] = (encoded_string[i+0] <= 'z') ? from_base64[encoded_string[i+0]] : 0xff;
        b4[1] = (encoded_string[i+1] <= 'z') ? from_base64[encoded_string[i+1]] : 0xff;
        b4[2] = (encoded_string[i+2] <= 'z') ? from_base64[encoded_string[i+2]] : 0xff;
        b4[3] = (encoded_string[i+3] <= 'z') ? from_base64[encoded_string[i+3]] : 0xff;

        // Transform into a group of three bytes
        BYTE b3[3];
        b3[0] = ((b4[0] & 0x3f) << 2) + ((b4[1] & 0x30) >> 4);
        b3[1] = ((b4[1] & 0x0f) << 4) + ((b4[2] & 0x3c) >> 2);
        b3[2] = ((b4[2] & 0x03) << 6) + ((b4[3] & 0x3f) >> 0);

        // Add the byte to the return value if it isn't part of an '=' character (indicated by 0xff)
        if (b4[1] != 0xff) ret.push_back(b3[0]);
        if (b4[2] != 0xff) ret.push_back(b3[1]);
        if (b4[3] != 0xff) ret.push_back(b3[2]);
    }

    return ret;
}

std::string base64::fileContentEncoded(const std::string& filepath)
{
    auto content = files::bContent(filepath);
    return encode(content);
}

