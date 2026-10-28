#include "./BaseArgv.h"

namespace ml
{
    bool BaseArgv::has(const std::string& key)
    {
        return _argv.find(key) != _argv.end();
    }
}
