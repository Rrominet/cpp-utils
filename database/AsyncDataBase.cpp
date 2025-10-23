#include "./AsyncDataBase.h"
#include "./DataBaseObject.h"

namespace ml
{
    void AsyncDataBase::setMaxMemory(long maxMemory)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _maxMemory = maxMemory;
    }

    void AsyncDataBase::removeAllIfTooMuchMemory()
    {
        long memory = 0;
        for (auto o : _objects)
            memory += o.second->size();

        db_write2("Memory used : ", memory);
        db_write2("Max memory : ", _maxMemory);

        if (memory > _maxMemory)
        {
            _objects.clear();
            db_write("Memory removed, emitting events.");
            _events.emit("memory-removed");
        }
    }

    void AsyncDataBase::remove(const std::string& path)
    {
        AbstractDataBase::remove(path);	
        auto f = [path]()
        {
            files::remove(path);
        };

        _pool.run(std::move(f));
    }

    void AsyncDataBase::save(const std::string &path)
    {
        try
        {
            this->save(_objects.at(path));
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("AsyncDataBase : Failed to save the path : " + std::string(e.what()));
        }
    }

    void AsyncDataBase::save(DataBaseObject* object)
    {
        auto f = [object]()
        {
            files::write(object->filepath(), object->stringifyied());
        };

        _pool.run(std::move(f));
    }
}
