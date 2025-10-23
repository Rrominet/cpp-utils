#include "./storage.h"
#include "./os.h"
#include "./files.2/files.h"

namespace storage
{
    bool _ensureDirsCalled = false;
    json _data;

    std::string path()
    {
        return os::home() + files::sep() + ".config" + files::sep() + files::execName() + files::sep() + "storage.json";        
    }

    void ensureDirs()
    {
        if (!files::isDir(os::home() + files::sep() + ".config"))
            files::mkdir(os::home() + files::sep() + ".config");

        if (!files::isDir(os::home() + files::sep() + ".config" + files::sep() + files::execName()))
            files::mkdir(os::home() + files::sep() + ".config" + files::sep() + files::execName());

        _ensureDirsCalled = true;
    }

    void save()
    {
        assert(_ensureDirsCalled && "storage::init() must be called before storage::save() - from storage::set()"); 
        files::write(path(), _data.dump());
    }

    void read()
    {
        if (!files::exists(path()))
        {
            lg("storage::read(): " + path() + " does not exist.");
            return;
        }

        try
        {
            _data = json::parse(files::read(path()));
        }
        catch(const std::exception& e)
        {
            lg(e.what());
        }
    }

    void init()
    {
        ensureDirs();
        read();
    }

    json& data()
    {
        assert(_ensureDirsCalled && "storage::init() must be called before storage::data() - from storage::get()");
        return _data;
    }
}
