#include "thread.h"
#include <chrono>
#include "vec.h"
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

void th::ThreadPool::threadRun()
{
    while(true) 
    {
        std::function<void()> job = 0;
        {
            std::unique_lock<std::mutex> lk(_threads.mtx());
            _qcond.wait(lk, [this]{
                    return !_jobs.empty() || _shouldStop;
                    });
            lg("threads woke up !");
            if (_shouldStop)
                return;

            lg("getting job");
            job = _jobs.front();
            _jobs.pop();
        }
        _running ++;
            lg("running job");
        job();
        _running --;
    }
}

th::ThreadPool::ThreadPool(int max)
{
    if (max > 0)
        _max = max;
    else
        _max = th::maxRunning();
    
    for (int i=0; i<_max; i++)
        _threads.push(std::thread(&ThreadPool::threadRun, this));
}

th::ThreadPool::~ThreadPool()
{
    this->stop();
}

void th::ThreadPool::run(const std::function<void ()> &f, const std::function<void ()> &callback)
{
    lg("ThreadPool::run");
    auto thfunc = [this, f, callback]
    {
        f();
        {
            std::lock_guard<std::mutex> lk(_cb_mtx);
            _callbacks.push_back(callback);
        }

        {
            std::lock_guard lk(_wakeupFuncMtx);
            if (_wakeupFunc)
            {
                lg("running wakeup event loop func.");
                _wakeupFunc();
            }
        }
    };
    _jobs.push(thfunc);
    lg("job pushed.");
    _qcond.notify_one();
    lg("threads notified.");
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
    {
        std::unique_lock<std::mutex> lk(_threads.mtx());
        _shouldStop = true;
    }
    _qcond.notify_all();
    for (auto& thread : _threads.vec)
        thread.join();
    _threads.clear();
}

void th::ThreadPool::processCallbacks()
{
    std::vector<std::function<void()>> cb;
    {
        std::lock_guard<std::mutex> lk(_cb_mtx);
        cb = _callbacks;
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

void th::sleep(int s)
{
    std::this_thread::sleep_for(std::chrono::seconds(s));
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
