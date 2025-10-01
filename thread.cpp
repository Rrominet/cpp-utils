#include "thread.h"
#include <chrono>
#include "vec.h"
#include <random>
#include <sstream>

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
            if (_shouldStop)
                return;

            job = _jobs.front();
            _jobs.pop();
        }
        _running ++;
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

void th::ThreadPool::run(const std::function<void ()> &f)
{
    lg("ThreadPool::run");
    _jobs.push(f);
    _qcond.notify_one();
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
