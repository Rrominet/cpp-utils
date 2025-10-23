#pragma once
#include "AbstractDataBase.h"
#include <atomic>
#include <cassert>
#include <memory>
#include "../thread.h"
#include "../vec.h"
#include "../Events.h"

namespace ml
{
    class DataBaseObject;
    class AsyncDataBase : public AbstractDataBase
    {
        public:
        AsyncDataBase(int threads=-1) : _pool(threads) {}
        virtual ~AsyncDataBase() = default;

        void setMaxMemory(long maxMemory);
        void removeAllIfTooMuchMemory();

        virtual void remove(const std::string& path) override;
        //
            // it will create the object in the database (file and memory) if not existing.
            // crash if it already exists in memory (use the update function to update an existing one.)
            // will return the one in files if existing
            // the container is a list-like<unique_ptr<T>> container containing the objects.
            // the created/getted object will be added to the container.
            // the container is responsible to delete the object using unique_ptr
            // the object WILL BE added to the container via push_back if insertIdx==-1 / if not inserted at the insertIdx/ if insertIdx > container.size(), it will be push_back
            // Args is the arguments of the constructor of T
            // T need to be a DataBaseObject or else it will crash.
            template<typename T, typename Container, typename ...Args>
                T* create(Container& container, int insertIdx, Args&& ...args)
                {
                    this->removeAllIfTooMuchMemory();
                    auto ptr = new T(std::forward<Args>(args)...);
                    auto datab_ptr = (ml::DataBaseObject*)ptr;
                    bool founded = this->update(&datab_ptr);
                    if (founded)
                        assert(false && "Object already in database, this should not happen.");

                    auto obj = std::unique_ptr<T>((T*)datab_ptr);
                    T* _r = nullptr;
                    if (insertIdx == -1 || insertIdx >= container.size())
                    {
                        container.push_back(std::move(obj));
                        _r = container.back().get();
                    }
                    else 
                    {
                        container.insert(container.begin() + insertIdx, std::move(obj));
                        _r = container[insertIdx].get();
                    }

                    this->save(_r);
                    return _r;
                }

            //these are async.
            //careful here, you do not know when this will finish... Do not delete the DataBaseObject right after calling this.
            void save(const std::string &path);
            void save(DataBaseObject* object);

            ml::Events& events() { return _events; }

        private : 
            th::ThreadPool _pool;
            long _maxMemory = 1'000'000'000; // 1Go

            ml::Events _events;
    };
}
