#pragma once
#include <string>

// pl stand for process-launcher
// this is an API to speak with the process-launcher server. 
// It must be installed and running on the system for it to work
// It's usefull to lunch process from another starting point (here the process-launcher server) (specially woth apache)
namespace pl
{
    void spawn(const std::string& cmd);
}
