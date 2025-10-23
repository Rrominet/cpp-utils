#include "./Observer.h"
#include "./Observed.h"

namespace ml
{
    Observer::~Observer()
    {
        if (_observed)
            _observed->removeObserver(this);
    }
    void Observer::update(const std::string& type, const std::any& data)
    {
        lg("Observer::update " + type);
        lg("Observer::update " + std::to_string(_onUpdate[type].size()) + " callbacks");
        for (const auto& func : _onUpdate[type])
            func(data);
    }

    void Observer::addOnUpdate(const std::string& type, const std::function<void(const std::any&)>& func, Observed* observed)
    {
        _onUpdate[type].push_back(func);
        if (observed)
        {
            observed->addObserver(this, type);
            assert(_observed == observed && "Observer::addOnUpdate : observed != _observed, this should not append, there is an arror in the Observed::addOberser(...) implementation");
        }
    }

    void Observer::removeOnUpdate(const std::string& type)
    {
        _onUpdate.erase(type);	
    }
}
