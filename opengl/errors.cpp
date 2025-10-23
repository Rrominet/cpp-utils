#include "errors.h"

void gl::clearError()
{
    while(glGetError() != GL_NO_ERROR);
}

bool gl::log(const char* func, const char* file, const int& line)
{
    while(GLenum error = glGetError())
    {
        std::cout << "[OpenGL Error] --> " << error << std::endl;
        std::cout << "Errors occurs for the function : " << func << std::endl;
        std::cout << "On the file : " << file << std::endl;
        std::cout << "At the line : " << line << std::endl << std::endl;

        std::cin.get();
        return false;
    }
    return true;
}
