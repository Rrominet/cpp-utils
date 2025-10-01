#pragma once
#include <string>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace boost::interprocess;
namespace shared
{
    shared_memory_object get(const char* name);

    struct Mem
    {
        shared_memory_object _m;
        mapped_region _reg;
        Mem(const char* name);
        ~Mem();
    };
}
