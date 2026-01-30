#include "thread.h"
#include <chrono>
#include "vec.h"
#include <exception>
#include <mutex>
#include <random>
#include <sstream>

namespace threads
{
    std::thread::id _main_id = std::this_thread::get_id();
    bool is_main()
    {
        auto cr = std::this_thread::get_id();
        lg("Main Thread ID : " << _main_id << " and Current Thread ID : " << cr);
        return cr == threads::_main_id;
    }
}

namespace th
{
#ifdef mydebug
    Mutex::Mutex(const std::string& name): _mtx(), _name(name)
    {
    }

    void Mutex::lock()
    {
        auto cid = std::this_thread::get_id();
        lg("locking mutex : " << _name << " from thread : " << cid);
        if (_owner.load() == cid)
        {
            lg("mutex : " << _name << " already locked by the same thread : " << cid);
            std::terminate();
        }
        _mtx.lock(); 
        _timer = std::chrono::high_resolution_clock::now();
        _owner.store(cid);
        lg("" << _name << " mutex locked.");
    }

    bool Mutex::try_lock()
    {
        auto cid = std::this_thread::get_id();
        lg("Trying to lock mutex : " << _name << " from thread : " << cid);
        if (_owner.load() == cid)
        {
            lg("mutex (try_lock) : " << _name << " already locked by the same thread : " << cid);
            std::terminate();
        }
        bool res = _mtx.try_lock(); 
        if (res)
        {
            _timer = std::chrono::high_resolution_clock::now();
            lg("" << _name << " mutex locked.");
            _owner.store(cid);
        }
        else 
            lg("" << _name << " mutex not locked.");
        return res;
    }

    void Mutex::unlock()
    {
        auto cid = std::this_thread::get_id();
        lg("unlocking mutex : " << _name << " from thread : " << cid);
        if (_owner.load() == std::thread::id())
        {
            lg("mutex (unlock) : " << _name << " not locked by any thread.");
            std::terminate();
        }

        if (_owner.load() != cid)
        {
            lg("mutex (unlock) : " << _name << " locked by another thread : " << _owner.load());
            std::terminate();
        }

        _owner.store(std::thread::id());
        _mtx.unlock(); 
        auto now = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - _timer).count();
        lg("" << _name << " mutex unlocked.");
        lg("Mutex " << _name << " was locked for " << us << " us.");
    }

    void ThreadChecker::check()
    {
        if (std::this_thread::get_id() != _id)
        {
            lg("The call of this fucntion is not from the same thread.");
            lg("Called from thread : " << std::this_thread::get_id());
            lg("Should be called from thread : " << _id);
            std::terminate();
        }
    }
#else
    Mutex::Mutex(const std::string& name): _mtx(){}
    void Mutex::lock(){_mtx.lock();}
    bool Mutex::try_lock(){return _mtx.try_lock();}
    void Mutex::unlock(){_mtx.unlock();}
    void ThreadChecker::check(){}

#endif
}


th::ThreadPool::ThreadPool(int max): _sync("th::ThreadPool"), _max(max){}

void th::ThreadPool::_init()
{
    lg("th::ThreadPool::_init");
    if (_max < 1)
        _max = th::maxRunning();

    lg(_max);
    std::lock_guard l(_sync);
    for (int i=0; i<_max; i++)
        _sync.data().threads.push(std::thread(&ThreadPool::threadRun, this));

    _sync.data().initialized = true;
}

void th::ThreadPool::threadRun()
{
    lg("th::ThreadPool::threadRun (in thread : " << std::this_thread::get_id() << ")");
    while(true) 
    {
        std::function<void()> job = 0;
        {
            std::unique_lock lk(_sync);
            _qcond.wait(lk, [this]{
                    return (!_sync.data().jobs.empty() || _sync.data().shouldStop);
                    });
            if (_sync.data().shouldStop)
                return;

            job = _sync.data().jobs.front();
            _sync.data().jobs.pop();
        }
        _running ++;
        job();
        _running --;
    }
}

th::ThreadPool::~ThreadPool()
{
    lg("th::ThreadPool::~");
    bool initialized = false;
    {
        std::lock_guard l(_sync);
        initialized = _sync.data().initialized;
    }
    if (initialized)
        this->stop();
}

void th::ThreadPool::run(const std::function<void ()> &f, const std::function<void ()> &callback)
{
    lg("th::ThreadPool::run");
    bool initialized = false;
    {
        std::lock_guard l(_sync);
        initialized = _sync.data().initialized;
    }
    if (!initialized)
        _init();

    auto thfunc = [this, f, callback]
    {
        f();
        {
            std::lock_guard lk(_cb_mtx);
            _callbacks.push_back(callback);
        }

        {
            std::lock_guard lk(_wakeupFuncMtx);
            if (_wakeupFunc)
                _wakeupFunc();
        }
    };

    {
        std::lock_guard lk(_sync);
        _sync.data().jobs.push(thfunc);
    }
    _qcond.notify_one();
}

void th::ThreadPool::setWakeupFunc(const std::function<void ()> &f)
{
    {
        std::lock_guard lk(_wakeupFuncMtx);
        _wakeupFunc = f;
    }
}

void th::ThreadPool::stop()
{
    lg("th::ThreadPool::stop");
    {
        std::lock_guard lk(_sync);
        _sync.data().shouldStop = true;
    }

    lg("notifying threads");
    _qcond.notify_all();

    lg("deleting threads");
    for (auto& thread : _sync.data(true).threads)
    {
        if (thread.joinable())
        {
            lg("joining thread : " << thread.get_id());
            thread.join();
        }
    }

    {
        std::lock_guard lk(_sync);
        _sync.data().threads.clear();
        _sync.data().initialized = false;
    }
}

void th::ThreadPool::processCallbacks()
{
    std::vector<std::function<void()>> cb;
    {
        std::lock_guard<std::mutex> lk(_cb_mtx);
        cb = std::move(_callbacks);
        _callbacks = std::vector<std::function<void()>>();
    }
    for (auto& callback : cb)
        callback();
}

int th::maxRunning()
{
    return std::thread::hardware_concurrency();
}

int th::maxSystem()
{
    return maxRunning();
}

void th::sleep(double s)
{
    int ms = s * 1000.0;
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void th::msleep(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void th::microsleep(int ms)
{
    std::this_thread::sleep_for(std::chrono::microseconds(ms));
}

std::string th::id()
{
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    std::string idstr = ss.str();
    return idstr;
}
