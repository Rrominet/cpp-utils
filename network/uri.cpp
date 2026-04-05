#include "uri.h"
#include "str.h"
#include <map>
#include <iostream>

namespace uri
{
    std::map<std::string, std::string> refs;
    bool setted = false;

    std::string decode(std::string data)
    {
        std::string result;
        result.reserve(data.size());
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (data[i] == '%' && i + 2 < data.size())
            {
                int hex = std::stoi(data.substr(i + 1, 2), nullptr, 16);
                result += static_cast<char>(hex);
                i += 2;
            }
            else if (data[i] == '+')
                result += ' ';
            else
                result += data[i];
        }
        return result;
    }

    std::string encode(std::string data)
    {
        std::string result;
        result.reserve(data.size() * 3);
        const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
        for (unsigned char c : data)
        {
            if (unreserved.find(c) != std::string::npos)
                result += c;
            else
            {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", c);
                result += buf;
            }
        }
        return result;
    }

    std::string test()
    {
        auto res = uri::decode("https://teach.motion-live.com/formations/H3D2%20:%20Le%20Guide%20du%20Voyageur%203D/");
        std::cout << res << std::endl;
        return res;
    }
}
