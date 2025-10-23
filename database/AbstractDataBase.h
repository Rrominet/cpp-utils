#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include "./files.2/files.h"

//this does contain a map _objects referencing all objects from their id (often their fullpath on os).
//but it does not own them, the memory management is done by the class that actually contains the DataBaseObject objects.

namespace ml
{
    class DataBaseObject;
    class AbstractDataBase
    {
        public : 
            AbstractDataBase() = default;
            virtual ~AbstractDataBase() = default;
        
            void add(DataBaseObject* object);

            // this should remove the data fron memory (and from files too)
            // the AbstractDataBase implementation only remove it from memory
            virtual void remove(const std::string& path);

            // this should be the method that read in file if necesseary/or update the object from memory if alreay loaded.
            // the default implementation does just that : 
            // if the object is not in memory, it will be loaded from file.
            // if it is in memory, it will do nothing because it sould be already up to date.
            virtual bool update(DataBaseObject** object);

            // the basic implementation only list the files in directory on disk
            // not in memory
            virtual std::vector<std::string> listDir(const std::string& path, files::SortType sort = files::NAME);

        protected : 
            std::unordered_map<std::string, DataBaseObject*> _objects;
            std::mutex _mtx;
    };
}
