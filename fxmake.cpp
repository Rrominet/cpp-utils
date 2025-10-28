#include "./fxmake.h"
#include "./files.2/files.h"
#include "debug.h"

namespace fxmake
{
    std::string _version = "unknown";
    json _dependencies = json::object();
    bool _init = false;

    void init()
    {
        if (_init)
            return;
        if (files::exists(files::execDir() + files::sep() + "version"))
            _version = files::read(files::execDir() + files::sep() + "version");
        if (files::exists(files::execDir() + files::sep() + "dependencies"))
        {
            try
            {
                _dependencies = json::parse(files::read(files::execDir() + files::sep() + "dependencies"));
            }
            catch(const std::exception& e)
            {
                lg(e.what());
            }
        }
        _init = true;
    }

    std::string version()
    {
        init();
        return _version;
    }

    json dependencies()
    {
        init();
        return _dependencies;
    }
}
