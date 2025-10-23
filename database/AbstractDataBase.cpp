#include "./AbstractDataBase.h"
#include "./DataBaseObject.h"
#include "../debug.h"

namespace ml
{
    void AbstractDataBase::add(DataBaseObject* object)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _objects[object->filepath()] = object;
    }

    void AbstractDataBase::remove(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _objects.erase(path);
    }

    std::vector<std::string> AbstractDataBase::listDir(const std::string& path,files::SortType sort)
    {
        return files::ls(path, sort);    
    }

    bool AbstractDataBase::update(DataBaseObject** object)
    {

        bool founded = false;
        {
            std::lock_guard<std::mutex> lock(_mtx);
            founded = _objects.find((*object)->filepath()) != _objects.end();
        }

        if (founded)
        {
            auto o = &_objects[(*object)->filepath()];
            if (*o != *object)
            {
                delete *object;
                object = o;
            }
            return true;
        }
        else 
        {
            // object not found in map
            {
                std::lock_guard<std::mutex> lock(_mtx);
                _objects[(*object)->filepath()] = *object;
            }
            if (!files::exists((*object)->filepath()))
                return false;

            std::string content;
            try
            {
                content = files::read((*object)->filepath());
                (*object)->deserialize(json::parse(content));
            }
            catch(const std::exception& e)
            {
                db_write2("Error reading file (or deserializing it) : " + (*object)->filepath(), e.what());
                db_write2("File content readed", content);
            } 

            return false;
        }
    }

}

