#pragma once

#include <GL/glew.h>

#ifdef mydebug
#define gl_call(x); gl::clearError();\
        x;\
        gl::log(#x, __FILE__, __LINE__);
#else
#define gl_call(x) x
#endif

#include <iostream>

namespace gl
{
    void clearError();
    bool log(const char* func, const char* file, const int& line);
}
