#pragma once 
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <map>
#include <shared_mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include "vec.h"

#include "debug.h"

#define sFutur th::Futur
#define th_guard std::lock_guard lk
#define LK std::lock_guard<std::mutex> lk
#define th_shard std::lock_guard lk
#define th_sguard std::lock_guard lk
#define th_shared std::shared_lock lk

// careful the fact that the MACRO create a variable lk is hidden, but must be use after... not the best way to do that...
#define try_shared(x) std::shared_lock lk(x, std::try_to_lock)
#define try_guard(x) std::unique_lock lk(x, std::try_to_lock)
#define try_sguard(x) std::shared_lock lk(x, std::try_to_lock)

namespace th
{
    enum State
    {
        NOT_STARTED,
        RUNNING, 
        FINISHED,
        _ERROR,
    };

    template <typename T>
        class Safe
        {
            private : 
                T _data;
                std::shared_mutex _mtx;

            public : 
                Safe& operator=(const T& other)
                {
                    {
                        th_shard(_mtx);
                        _data = other;
                    }
                    return *this;
                }

                Safe& operator+=(const T& other)
                {
                    {
                        th_shard(_mtx);
                        _data += other;
                    }
                    return *this;
                }

                Safe& operator-=(const T& other)
                {
                    {
                        th_shard(_mtx);
                        _data -= other;
                    }
                    return *this;
                }

                operator T () {th_shared(_mtx); return _data;}

                T get()
                {
                    th_shared(_mtx);
                    return _data;
                }
        };

    struct Futur
    {
        Safe<std::string> infos;
        Safe<std::string> allInfos;
        Safe<std::string> error;
        std::atomic<float> pgr = 0.0;
        std::atomic<long long> count = 0;
        std::atomic<long long> count2 = 0;
        std::atomic<long long> count3 = 0;
        std::atomic<long long> count4 = 0;
        std::atomic<long long> total = 0;
        std::atomic<bool> needToQuit = false;
        std::map<std::string, std::string> moreInfos;
        std::atomic<State> state = NOT_STARTED;

        float readPgr(){return pgr;}
        long long readCount(){return count;}
        long long readTotal(){return total;}
    };

    int maxRunning();

    //alias maxRunning
    int maxSystem();
    std::string id();

    // make the current thread sleep for s seconds
    void sleep(int s);
    // make the current thread sleep for ms miliseconds
    void msleep(int ms);
    // make the current thread sleep for ms microseconds
    void microsleep(int ms);


}

namespace threads
{
    template<typename F, typename ...Args>
        void launch(F f, Args... args)
        {
            std::thread(f, args...).detach();
        }

    template<typename F, typename ...Args>
    void start(F f, Args... args){launch(f, args...);}

    //it will create the object on a thread
    //the callbak take the created object ptr as arg
    //don't use the object before the callback hasbeen called
    template<typename T, typename... As>
        void createLater(std::function<void (T*)> callback=0, As... args)
        {
            auto f = [=]()
            {
                auto obj = new T(args...);
                if (callback)
                    callback(obj);
            };
            threads::launch(f);
        }

    // it will delete the object on a thread.
    // be careful here the object can't have active reference after this call
    template<typename T>
        void deleteLater(T* toDel, std::function<void ()> callback=0)
        {
            auto f = [=]()
            {
                delete toDel;
                if (callback)
                    callback();
            };
            threads::launch(f);
        }

    bool is_main();

    template<typename F, typename ...Args>
    void run(F f, Args... args)
    {
        auto tocall = [=]
        {
            f(args...);
        };
    }
}
namespace ml
{
    template<typename T>
    class thVec
    {
        private : 
            mutable std::mutex _mtx;
        public : 
            Vec<T> vec;

            thVec() : vec(){}
            thVec(const std::vector<T>& pvec) : vec(pvec){}
            thVec(std::vector<T>&& pvec) : vec(pvec){}

            thVec(const Vec<T>& pvec) : vec(pvec){}
            thVec(Vec<T>&& pvec) : vec(pvec){}

            thVec(const thVec<T>& other) : vec(other.vec){}

            thVec(std::initializer_list<T> list) : vec(list){}
            
            // return copy of object to force this to be thread-safe
            T at(size_t idx){th_guard(_mtx); return vec.at(idx);}
            T operator[](size_t idx){th_guard(_mtx); return vec[idx];}

            Vec<T>& operator=(const Vec<T>& other){th_guard(_mtx); vec = other.vec; return *this;}
            Vec<T>& operator=(Vec<T>&& other){th_guard(_mtx); vec = other.vec; return *this;}

            const size_t size()const {th_guard(_mtx); return vec.size();}
            const size_t length()const {th_guard(_mtx); return vec.size();}

            // return copy of object to force this to be thread-safe
            T first() {th_guard(_mtx); return vec[0];}
            T front() {th_guard(_mtx); return vec[0];}
            T last() {th_guard(_mtx); return vec[this->size() - 1];}
            T back() {th_guard(_mtx); return vec[this->size() - 1];}

            void push_back(const T& v){th_guard(_mtx); vec.push_back(v);}
            void push_back(T&& v){th_guard(_mtx); vec.push_back(std::move(v));}

            void push(const T& v){th_guard(_mtx); vec.push_back(v);}
            void push(T&& v){th_guard(_mtx); vec.push_back(std::move(v));}

            void add(const T& v){th_guard(_mtx); vec.add_back(v);}
            void add(T&& v){th_guard(_mtx); vec.add_back(std::move(v));}

            void append(const T& v){th_guard(_mtx); vec.append_back(v);}
            void append(T&& v){th_guard(_mtx); vec.append_back(std::move(v));}

            void pop_back(){th_guard(_mtx); vec.pop_back();}
            T pop()
            {
                th_guard(_mtx); 
                if (vec.size()==0)
                    throw(std::length_error("The size of the vector is 0."));
                T _r(this->last());
                vec.pop_back();
                return _r;
            }

            bool operator==(const thVec<T>& other) const
            {
                th_guard(_mtx); 
                if (vec.size() != other.size())
                    return false;

                for (int i=0; i<this->size(); i++)
                {
                    if (vec[i] != other[i]) 
                        return false;
                }

                return true;
            }

            bool operator!=(const Vec<T>& other) const
            {
                return !(other == *this);
            }

            int remove(const T& elmt)
            {
                th_guard(_mtx);
                return vec.remove(elmt);
            }
            
            // use when T is a pointer !
            int del(T elmt)
            {
                th_guard(_mtx);
                return vec.del(elmt);
            }

            T shift(){th_guard(_mtx); return vec.shift();}

            void prepend(const T& v){th_guard(_mtx); vec.prepend(v);}
            void prepend(T&& v){th_guard(_mtx); vec.prepend(std::move(v));}
            void clear(){th_guard(_mtx); vec.clear();}

            bool includes(const T& elmt){th_guard(_mtx); return vc::includes(vec, elmt);}
            bool contains(const T& elmt){th_guard(_mtx); return this->includes(elmt);}

            int find(const T& elmt){th_guard(_mtx); return vc::find(vec, elmt);}

            void iterate(std::function<void (const T& elmt)> f) const
            {
                th_guard(_mtx);
                vec.iterate(f);
            }

            void iterate(std::function<void (T& elmt)> f)
            {
                th_guard(_mtx);
                vec.iterate(f);
            }

            void execOnOne(std::function<void (const T& elmt)> f, size_t idx) const
            {
                th_guard(_mtx);
                f(vec[idx]);
            }

            void execOnOne(std::function<void (T& elmt)> f, size_t idx)
            {
                th_guard(_mtx);
                f(vec[idx]);
            }

            std::mutex& mtx(){return _mtx;}
    };
}

namespace th 
{
    class ThreadPool
    {
        private : 
            unsigned int _max = 0;
            std::condition_variable _qcond;
            ml::thVec<std::thread> _threads;
            std::queue<std::function<void()>> _jobs;
            bool _shouldStop = false;
            std::atomic_int _running = 0;

            std::vector<std::function<void()>> _callbacks;
            std::function<void ()> _wakeupFunc = 0;
            std::mutex _wakeupFuncMtx;
            std::mutex _cb_mtx;

            void threadRun();

        public : 
            ThreadPool(int max = -1);
            ~ThreadPool();

            //the callback will be executed on the ThreadPool thread when calling processCallbacks();
            //processCallbacks() should be called in you event loop in each pass
            void run(const std::function<void ()> &f, const std::function<void ()> &callback=0);
            void stop();
            void processCallbacks();

            //the function passed here will be executed each time a functon is pushed to the _callbacks queue.
            //You tipicily would use it to signal your event loop to wake up and calling processCallbacks().
            //In a GUI app, it would be something like processEvents() or queueEvent()...
            //in mlgui.2, it's simply a call to ml::app()->queue([]{yourpool.processCallbacks();}).
            //in ipc it would simply a be a call to ipc::signal() and having the pool.processCallbacks() directly in the ipc stdin read loop with ipc::addOnReadLoop() 
            void setWakeupFunc(const std::function<void ()> &f);

            size_t nbWaiting() {return _threads.size();}
            int nbRunning() {return _running;}

            int max(){return _max;}
            void setMax(int max){_max = max;}
    };
}
