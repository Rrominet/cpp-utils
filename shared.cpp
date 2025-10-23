#include "shared.h"

// this is a library for using shared memory between process
// for now is just a client side one.
//
// need the lib rt to work (no need to copy it just put in the target_link_libraries() of cMakeLists )

shared_memory_object shared::get(const char* name)
{
    shared_memory_object mem = shared_memory_object(open_only, name, read_write); 
    return mem;
}

using namespace shared; 

Mem::Mem(const char*name)
{
    _m = get(name); 
    _reg = mapped_region (_m, read_write);
}

Mem::~Mem()
{

}
