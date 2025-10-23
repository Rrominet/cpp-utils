#include "memory.h"
#include "debug.h"

namespace mem
{
    size_t allocated = 0;
    size_t freed = 0;
}

void* operator new(size_t size)
{
#ifdef debug
    mem::allocated += size; 
#endif
    return malloc(size); 
}

void operator delete (void* memory, size_t size)
{
#ifdef debug
    mem::freed += size; 
#endif
    free(memory); 
}

void mem::infos()
{
    using namespace mem;
#ifdef debug
    lg2("Total memory allocated (mB)", allocated * 0.000001);
    lg2("Total memory freed (mB)", freed * 0.000001);
    lg2("Memory still in use (mB)", (allocated - freed) * 0.000001);
#endif
}
