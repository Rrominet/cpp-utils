#include "./PeriodicDataBase.h"
#include "./DataBaseObject.h"
#include "../files.2/files.h"
#include "../debug.h"

namespace ml
{
    PeriodicDataBase::PeriodicDataBase(size_t msperiod) : _period(msperiod)
    {
        auto function = [this]()
        {
            std::unique_lock<std::mutex> lock(_destroyMtx);
            while (true)
            {
                // if destroying, stoping the loop and exiting the thread.
                if (_cv.wait_for(lock, std::chrono::milliseconds(_period)), _destroying)
                    return;
                this->save();
            }
        };
        _thread = std::make_unique<std::thread>(function);
    }

    PeriodicDataBase::~PeriodicDataBase()
    {
        if (!_thread)
            return;

        {
            std::lock_guard<std::mutex> lock(_destroyMtx);
            _destroying = true;
        }
        _cv.notify_all();
        _thread->join();
    }

    void PeriodicDataBase::setMaxMemory(long maxMemory)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _maxMemory = maxMemory;
    }

    void PeriodicDataBase::removeAllIfTooMuchMemory()
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

    void PeriodicDataBase::save()
    {
        std::unordered_map<std::string, DataBaseObject*> cp_objects;
        {
            std::lock_guard<std::mutex> lock(_mtx); 
            cp_objects = _objects;
        }
        for (auto it = cp_objects.begin(); it != cp_objects.end(); it++)
        {
            try
            {
                auto path = it->first;
                auto parent = files::parent(path);
                if (!files::isDir(parent))
                    files::mkdir(parent);
                files::write(it->first, it->second->stringifyied());
            }
            catch (const std::exception& e)
            {
                db_write2("Error writing file : " + it->first, e.what());
            }
        }

        {
            std::lock_guard<std::mutex> lock(_mtx);
            for (const auto& f : _toremove)
            {
                try
                {
                    bool res = files::remove(f);
                    if (!res)
                        db_write2("File does not exists, so can't delete it", f);
                }
                catch(const std::exception& e)
                {
                    db_write2("Error trying to remove file " + f, e.what());
                }
            }
            _toremove.clear();
        }

        {
            std::lock_guard<std::mutex> lk(_mtx);
            this->removeAllIfTooMuchMemory();
        }
    }

    void PeriodicDataBase::remove(const std::string& path)
    {
        AbstractDataBase::remove(path); 
        std::lock_guard<std::mutex> lock(_mtx);
        _toremove.push_back(path);
    }

}
