#pragma once
#include "str.h"
#include <functional>

namespace stds
{
    // newLine is executed each time a new line is read from stdin (the argument is the last line)
    void read_in(const std::function<void(const std::string&)> &newLine);
    void read_in_async(const std::function<void(const std::string&)> &newLine);
    // eof is executed when there is no more data to read and the stdin receives EOF signal (the argument here is all the data received)
    void read_in_async(const std::function<void(const std::string&)> &newLine, const std::function<void(const std::string&)> &eof);
}
