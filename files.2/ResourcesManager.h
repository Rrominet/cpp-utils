#pragma once
#include <string>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <future>
#include <functions.h>
#include <vector>
#include "../Events.h"
#include "../thread.h"

namespace ml
{
    // when loaded fire the loaded event on Resource::events()
    class Resource
    {
        protected : 
            std::atomic<bool> _loaded = false;
            std::vector<char> _data;
            Events _events;

        public : 
            virtual bool loaded() {return _loaded.load();}
            virtual void load(const std::string& filepath);

            const std::vector<char>& data() const;
            std::vector<char>& data();

            Events& events() {return _events;}
    };

    class ResourcesManager
    {
        public : 
            ResourcesManager() : _pool(32){}

            // this is asyncronous
            // all the moment your resource is not loaded, it will return Resource->loaded() = false
            // you can add a callback to be called when the resource is loaded
            // the call back will be executed on a different thread (on the one where the resource is loaded.)
            // so DO NOT call UI stuff in it ! (queue them instead on the mainthread event queue.)
            template<typename T>
                std::shared_ptr<T> get(const std::string& filepath, const std::function<void(Resource* )>& callback = 0)
                {
                    if (_resources.find(filepath) == _resources.end()) // not found
                    {
                        lg("resource not find, so we create it.");
                        auto res = std::make_shared<T>();
                        res->events().add("loaded", [callback, this, filepath](){callback(_resources[filepath].get());});
                        _resources[filepath] = res;
                        auto todo = [this, filepath]
                        {
                            _resources[filepath]->load(filepath);
                        };
                        _pool.run(todo);
                    }
                    else if (_resources[filepath]->loaded())
                        callback(_resources[filepath].get());
                    else 
                        _resources[filepath]->events().add("loaded", [callback, this, filepath](){callback(_resources[filepath].get());});

                    return std::static_pointer_cast<T>(_resources[filepath]);
                }
            int maxConcurrency(){return _pool.max();}
            void setMaxConcurrency(int max){_pool.setMax(max);}

        protected:
            std::unordered_map<std::string, std::shared_ptr<Resource>> _resources;
            th::ThreadPool _pool;
    };
}
