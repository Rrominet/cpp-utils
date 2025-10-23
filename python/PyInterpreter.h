#pragma once

#include <string>
#include <Python.h>

class PyInterpreter
{
    public :
        PyInterpreter(); 
        ~PyInterpreter();

        void run(const std::string &pyCode);
        bool runFromFile(const std::string &path);
};
