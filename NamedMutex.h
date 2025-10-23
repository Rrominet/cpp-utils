#pragma once
#include <mutex>
#include <string>

namespace ml
{
    class NamedMutex
    {
        public : 
            NamedMutex(const std::string& name);
            void lock();
            void unlock();
            bool try_lock();

        private : 
            std::mutex _m;
            std::string _name;
    };
}
