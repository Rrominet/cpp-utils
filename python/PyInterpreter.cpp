#include "PyInterpreter.h"

PyInterpreter::PyInterpreter()
{
    Py_Initialize();
}

PyInterpreter::~PyInterpreter()
{
    Py_FinalizeEx();
}

void PyInterpreter::run(const std::string &pyCode)
{
    PyRun_SimpleString(pyCode.c_str());
}

bool PyInterpreter::runFromFile(const std::string &path)
{
    FILE *fp = fopen(path.c_str(), "r");
    if (!fp)
        return false;

    PyRun_SimpleFile(fp, path.c_str());
    return true;
}
