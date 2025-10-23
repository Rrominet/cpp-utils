#include "./NamedMutex.h"
#include "debug.h"

namespace ml
{
    NamedMutex::NamedMutex(const std::string& name) : _name(name)
    {

    }

    void NamedMutex::lock()
    {
        lg("locking : " << _name); 
        _m.lock();
        lg("" << _name << " locked.");
    }

    void NamedMutex::unlock()
    {
        lg("unlocking : " << _name); 
        _m.unlock();
        lg(_name << " unlocked.");
    }

    bool NamedMutex::try_lock()
    {
        lg("Trying to lock : " << _name); 
        return _m.try_lock();
    }
}
