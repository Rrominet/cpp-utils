#include "stds.h"
#include <thread>

#include <iostream>
#include <sys/eventfd.h>
#include <poll.h>
#include <unistd.h>
#include <string>
#include <cstring>

namespace stds
{
    int _efd = -1;
    struct pollfd _fds[2];
    // newLine is executed each time a new line is read from stdin (the argument is the last line)
    //
    //
    int efd()
    {
        return _efd;
    }

    void read_in(const std::function<void(const std::string&)> &newLine)
    {
        _fds[0].fd = STDIN_FILENO;
        _fds[0].events = POLLIN;
        _fds[1].fd = _efd;
        _fds[1].events = POLLIN;

        std::string line;
        while (true)
        {
            int ret = poll(_fds, 2, -1);
            if (ret < 0)
            {
                perror("poll");
                break;
            }
            if (_fds[0].revents & POLLIN)
            {
                if(std::getline(std::cin, line))
                    newLine(line);
                else 
                    break;
            }
            if (_fds[1].revents & POLLIN)
            {
                uint64_t value;
                read(_efd, &value, sizeof(value));
                newLine("");
            }
        }

        close(_efd);
    }
    void read_in_async(const std::function<void(const std::string&)> &newLine)
    {
        auto f = [newLine]
        {
            stds::read_in(newLine);
        };

        std::thread(f).detach();
    }
    // eof is executed when there is no more data to read and the stdin receives EOF signal (the argument here is all the data received)
    void read_in_async(const std::function<void(const std::string&)> &newLine, const std::function<void(const std::string&)> &eof)
    {
        auto f = [newLine, eof]
        {
            std::string all;
            auto cb = [newLine, &all](const std::string& line)
            {
                std::string all;
                newLine(line);
                all += line + "\n";
            };
            stds::read_in(cb);
            if (eof)
                eof(all); 
        };

        std::thread(f).detach();
    }
}
