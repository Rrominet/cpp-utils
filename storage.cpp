#include "./storage.h"
#include "./os.h"
#include "./files.2/files.h"
#include "./files.2/AsyncFilesystem.h"

namespace storage
{
    bool _ensureDirsCalled = false;
    json _data;
    std::unique_ptr<ml::AsyncFilesystem> _fs;

    std::string path()
    {
        return "storage.json";        
    }

    void ensureDirs()
    {
        if (!files::isDir(os::home() + files::sep() + ".config"))
            files::mkdir(os::home() + files::sep() + ".config");

        if (!files::isDir(os::home() + files::sep() + ".config" + files::sep() + files::execName()))
            files::mkdir(os::home() + files::sep() + ".config" + files::sep() + files::execName());

        _ensureDirsCalled = true;
    }

    void save(const std::function<void(size_t)>& cb, const std::function<void(const std::string&)>& error)
    {
        assert(_ensureDirsCalled && "storage::init() must be called before storage::save() - from storage::set()"); 
        _fs->write(path(), _data.dump(), cb, error);
    }

    ml::Ret<size_t> save_sync()
    {
        assert(_ensureDirsCalled && "storage::init() must be called before storage::save() - from storage::set()"); 
        return _fs->write_sync(path(), _data.dump());
    }

    void read()
    {
        try
        {
            auto rdata = _fs->read_sync(path());
            if (!rdata.success)
            {
                lg("storage::read(): " + rdata.message);
                return;
            }
            _data = json::parse(rdata.value);
        }
        catch(const std::exception& e)
        {
            lg(e.what());
        }
    }

    void init()
    {
        _fs = std::make_unique<ml::AsyncFilesystem>();
        ensureDirs();
        _fs->setRoot(os::home() + files::sep() + ".config" + files::sep() + files::execName());
        read();
    }

    json& data()
    {
        assert(_ensureDirsCalled && "storage::init() must be called before storage::data() - from storage::get()");
        return _data;
    }
}
