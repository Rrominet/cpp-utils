#pragma once
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <any>
#include <mutex>
#include <condition_variable>

namespace ml
{
    class Events
    {        
        public : 
            Events(){}
            virtual ~Events(){}

            int add(const std::string &type, std::function<void()> func);
            void remove(const std::string &type, int id);
            void removeInAll(int id);
            void clear(const std::string &type);

            virtual void emit(const std::string &type, std::string data);

            // use this if you need your data to be copied, if not use the (void*) one
            // event if this function use &, the data is copied internally to _cdata.
            // you need to keep track of the type yourself to use it in std::any_cast
            virtual void emit(const std::string &type, const std::any& data=0);

            std::any data() {return _data;}
            const std::any& data()const {return _data;}

            template <typename T>
            T data() {return std::any_cast<T>(_data);}

            template <typename T>
            const T& data() const {return std::any_cast<T>(_data);}
            
            //return custom data as a copy
            //same as data() but kept for compatibility
            //new code should use data();
            std::any cdata(){return _data;}
            std::string stringData(){return std::any_cast<std::string>(_data);}

            template<typename... Ints>
                void removeAll(Ints... handles)
                {
                    std::vector<int> ids = {handles...};
                    for (auto id : ids)
                        this->removeInAll(id);
                }

            // this will block the emission of events until allow() is called
            void block(){_allow = false;}
            void allow(){_allow = true;}

            // will block until a event is emitted. (typically from another thread, this won't work if you have only one thread in your entire program)
            // it does not execute any code from the listener, it just wait.
            // it's a mecanism to stop your code flow until an event occurs.
            void wait(const std::string& type);

            //return the event emitted
            std::string wait(const std::vector<std::string>& types);

        private : 
            std::unordered_map<std::string, std::unordered_map<int, std::function<void()>>> _listeners;
            std::any _data;
            int _id = 1;
            bool _allow = true;
            
            // Synchronization primitives
            std::mutex _mutex;
            std::condition_variable _condition;
            std::string _lastEventType = "";
            int _eventCounter = 0; // To track the number of emits for synchronization so that waiting threads only exec once per emit() call
    };
}
