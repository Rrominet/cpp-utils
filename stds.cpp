#include "stds.h"
#include <thread>

namespace stds
{
    // newLine is executed each time a new line is read from stdin (the argument is the last line)
    //
    //
    void read_in(const std::function<void(const std::string&)> &newLine)
    {
        std::string line;
        while(std::getline(std::cin, line))
            newLine(line);
    }
    void read_in_async(const std::function<void(const std::string&)> &newLine)
    {
        auto f = [newLine]
        {
            std::string line;
            while(std::getline(std::cin, line))
                newLine(line);
        };

        std::thread(f).detach();
    }
    // eof is executed when there is no more data to read and the stdin receives EOF signal (the argument here is all the data received)
    void read_in_async(const std::function<void(const std::string&)> &newLine, const std::function<void(const std::string&)> &eof)
    {
        auto f = [newLine, eof]
        {
            std::string line;
            std::string all;
            while(std::getline(std::cin, line))
            {
                newLine(line);
                all += line + "\n";
            }
            if (eof)
                eof(all); 
        };

        std::thread(f).detach();
    }
}
