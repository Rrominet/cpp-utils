#pragma once
#include "./str.h"
#include "./tconv.h"

template<typename Nb>
    Nb str::asNumber(const std::string& v)
    {
        return tconv::convert<std::string, Nb>(v);
    }
