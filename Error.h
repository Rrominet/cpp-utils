#pragma once
#include <string>
#include <iostream>
#include "debug.h"

struct Error
{
    std::string msg = "";
    int value = 0;
    double dvalue = 0.0;
    bool succeed = false;

    void out(){std::cout << msg << std::endl;}
    void setMsg(const std::string& m){msg = m; lg(m);}
};
