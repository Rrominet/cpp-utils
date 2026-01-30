#pragma once 
#include <chrono>
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
#include <optional>
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
#ifdef mydebug
    class Mutex
    {
        public :
            Mutex(const std::string& name="");
            ~Mutex() = default;

            void lock();
            bool try_lock();
            void unlock();
            std::string name() const{return _name;}

            std::optional<std::thread::id> owner(){return _owner;}

            std::mutex& mtx(){return _mtx;}

        private : 
            std::string _name;
            std::mutex _mtx;
            std::atomic<std::thread::id> _owner;
            std::chrono::time_point<std::chrono::high_resolution_clock> _timer;
    };

    class ThreadChecker
    {
        private : 
            std::thread::id _id;

        public : 
            ThreadChecker() : _id(std::this_thread::get_id()) {}
            ~ThreadChecker() = default;

            void check();
    };

#else
    class Mutex
    {
        public :
            Mutex(const std::string& name="");
            ~Mutex() = default;

            void lock();
            bool try_lock();
            void unlock();
            std::string name() const{return "";}
            std::mutex& mtx(){return _mtx;}

        private : 
            std::mutex _mtx;
    };

    class ThreadChecker
    {
        public : 
            ThreadChecker() = default;
            ~ThreadChecker() = default;
            void check();
    };
#endif

    template <typename T, typename M=th::Mutex>
        class Safe
        {
            public : 
                Safe(const std::string& mtxname="") : _mtx(mtxname) {}

                void lock(){_mtx.lock();}
                void unlock(){_mtx.unlock();}
                bool try_lock(){return _mtx.try_lock();}

                //if fuck_the_lock is true, it will not check the lock, could be useful if you want to access data without locking (but you need to know what you're doing)
                T& data(bool fuck_the_lock = false)
                {
#ifdef mydebug
                    if (fuck_the_lock)
                        return _data;
                    if (!_mtx.owner().has_value())
                    {
                        lg("mutex (data) : " << _mtx.name() << " not locked by any thread. This data should be protected by a lock.");
                        std::terminate();
                    }

                    auto cid = std::this_thread::get_id();
                    if (_mtx.owner().value() != cid)
                    {
                        lg("mutex (data) : " << _mtx.name() << " locked by another thread : " << _mtx.owner().value());
                        lg("Meaning you're trying to access the data from a different thread than the one that locked it. You need to lock it first.");
                        std::terminate();
                    }
#else
#endif
                    return _data;
                }

                M& mtx(){return _mtx;}

            private : 
                T _data;
                M _mtx;
        };

    //work only with Safe (not Mutex directly (to FIX))
    class Cond
    {
        std::condition_variable _cv;
        public:
        template<typename MutexW, typename Predicate>
            void wait(std::unique_lock<MutexW>& lock, Predicate pred) {
                auto& wrapper = *lock.mutex();

                while (!pred()) {
                    lock.unlock(); // Calls your wrapper's unlock (debug code runs)

                    std::unique_lock<std::mutex> internal_lock(wrapper.mtx().mtx());
                    _cv.wait(internal_lock);
                    internal_lock.unlock();

                    lock.lock(); // Calls your wrapper's lock (debug code runs)
                }
            }

        void notify_one() { _cv.notify_one(); }
        void notify_all() { _cv.notify_all(); }
    };



    int maxRunning();

    //alias maxRunning
    int maxSystem();
    std::string id();

    // make the current thread sleep for s seconds
    void sleep(double s);
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
    struct ThreadPoolSync
    {
        ml::Vec<std::thread> threads;
        std::queue<std::function<void()>> jobs;
        bool shouldStop = false;
        bool initialized = false;
    };

    class ThreadPool
    {
        private : 
            int _max = 0;
            Cond _qcond;
            std::atomic_int _running = 0;

            th::Safe<ThreadPoolSync> _sync;

            std::vector<std::function<void()>> _callbacks;
            std::function<void ()> _wakeupFunc = 0;
            std::mutex _wakeupFuncMtx;
            std::mutex _cb_mtx;

            void threadRun();

            void _init();

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

            size_t nbWaiting() {std::lock_guard lk(_sync); return _sync.data().threads.size();}
            int nbRunning() {return _running;}

            int max(){return _max;}
            void setMax(int max){_max = max;}
    };
}
