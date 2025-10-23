#include "./Events.h"
#include "./debug.h"
#include <mutex>

namespace ml
{
    int Events::add(const std::string &type, boost::function<void()> func)
    {
        if (_listeners.find(type) == _listeners.end()) 
            _listeners[type] = std::unordered_map<int, boost::function<void()>>();

        _listeners[type][_id] = func;
        int _tmp = _id;
        _id ++;
        return _tmp;
    }

    void Events::remove(const std::string &type, int id)
    {
        if (type.empty())
            this->removeInAll(id);

        for (auto it = _listeners[type].begin(); it != _listeners[type].end(); it++)
        {
            if (it->first == id)
            {
                _listeners[type].erase(it);
                break;
            }
        }
    }

    void Events::removeInAll(int id)
    {
        for (auto it = _listeners.begin(); it != _listeners.end(); it++)
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
            {
                if (it2->first == id)
                {
                    it->second.erase(it2);
                    break;
                }
            }
        }
    }

    void Events::clear(const std::string &type)
    {
        _listeners.erase(type);
    }

    void Events::emit(const std::string &type, std::string data)
    {
        this->emit(type, std::any(data));
    }

    void Events::emit(const std::string &type, const std::any& data)
    {
        if (!_allow)
        {
            lg("Event " + type + " blocked.");
            return;
        }

        _data = data;

        //notify all waiting threads
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _lastEventType = type;
            _eventCounter ++;
            _condition.notify_all();
        }

        if (_listeners.find(type) == _listeners.end()) 
        {
            lg("No listeners for " + type);
            return;
        }

        
        for (auto &func : _listeners[type])
            func.second();
    }

    void Events::wait(const std::string& type)
    {
        int lastEvent;

        std::unique_lock<std::mutex> lock(_mutex); 
        lastEvent = _eventCounter;

        _condition.wait(lock, [this, &type, &lastEvent]()
                {
                    return _lastEventType == type && _eventCounter > lastEvent;
                });
    }

    std::string Events::wait(const std::vector<std::string>& types)
    {
        int lastEvent;
        std::string _r;

        std::unique_lock<std::mutex> lock(_mutex); 
        lastEvent = _eventCounter;

        _condition.wait(lock, [this, &types, &lastEvent, &_r]()
                {
                    for (auto type : types)
                    {
                        if (_lastEventType == type && _eventCounter > lastEvent)
                        {
                            _r = type;
                            return true;
                        }
                    }
                    return false;
                });
        return _r;
    }
}
