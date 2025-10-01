#pragma once
#include "AbstractDataBase.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <thread>
#include <condition_variable>
#include "vec.h"
#include "Events.h"

namespace ml
{
    class DataBaseObject;
    class PeriodicDataBase : public AbstractDataBase
    {
        public:
            PeriodicDataBase(size_t msperiod = 1000);
            virtual ~PeriodicDataBase();

            // is executed on the loop thread, so it should be thread safe
            // it should not use the mtx _destroyMtx (its already locked when called);
            void save();
            
            // if the total memory in that database exeeds the maxMemory, all the objects will be removed in that database just after saving.
            // it will call an event "memory-removed" so you can subscribe to it and remove the objects in your container too.
            void setMaxMemory(long maxMemory);
            void removeAllIfTooMuchMemory();

            virtual void remove(const std::string& path) override;

            // it will create the object in the database (file and memory) if not existing.
            // crash if it already exists in memory (use the update function to update an existing one.)
            // will return the one in files if existing
            // the container is a list-like<unique_ptr<T>> container containing the objects.
            // the created/getted object will be added to the container.
            // the container is responsible to delete the object using unique_ptr
            // the object WILL BE added to the container via push_back if insertIdx==-1 / if not inserted at the insertIdx/ if insertIdx > container.size(), it will be push_back
            // Args is the arguments of the constructor of T
            template<typename T, typename Container, typename ...Args>
                T* create(Container& container, int insertIdx, Args&& ...args)
                {
                    auto ptr = new T(std::forward<Args>(args)...);
                    auto datab_ptr = (ml::DataBaseObject*)ptr;
                    bool changed = this->update(&datab_ptr);
                    if (changed)
                        assert(false && "Object already in database, this should not happen.");

                    auto obj = std::unique_ptr<T>((T*)datab_ptr);
                    if (insertIdx == -1 || insertIdx >= container.size())
                    {
                        container.push_back(std::move(obj));
                        return container.back().get();
                    }
                    else 
                    {
                        container.insert(container.begin() + insertIdx, std::move(obj));
                        return container[insertIdx].get();
                    }
                }

            ml::Events& events() {return _events;}

        private : 
            std::mutex _destroyMtx;
            std::condition_variable _cv;
            bool _destroying = false;
            //im ms
            size_t _period = 1000;
            std::unique_ptr<std::thread> _thread;
            ml::Vec<std::string> _toremove;

            ml::Events _events;
            long _maxMemory = 1'000'000'000; // 1Go
    };
}
