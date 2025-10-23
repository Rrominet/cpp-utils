#pragma once 
#include <cstddef>


void* operator new(size_t size); 
void operator delete (void* memory, size_t size) noexcept;

namespace mem
{
    void infos();
}
