#include "./Observed.h"
#include "./Observer.h"

namespace ml
{
    Observed::~Observed()
    {
        for (auto& obs : _observers)
        {
            for (auto observer : obs.second)
            {
                if (observer->_observed == this)
                    observer->_observed = nullptr;
            }
        }
    }

    void Observed::addObserver(Observer* observer, const std::string& type)
    {
        lg("Observed::addObserver " + type);
        auto& obs = _observers[type];
        if (!obs.contains(observer))
        {
            lg ("Observer not already in the list, so we add it...");
            obs.push(observer);
            if (observer->_observed)
            {
                lg ("Removing old observed object from the observer...");
                observer->_observed->removeObserver(observer);
            }
            observer->_observed = this;
            lg ("Observer added to : " + type);
        }

        lg("now " + std::to_string(obs.size()) + " observers for " + type);
    }

    void Observed::removeObserver(Observer* observer, const std::string& type)
    {
        auto& obs = _observers[type];
        if (obs.contains(observer))
            obs.remove(observer);
    }

    void Observed::removeObserverFromAll(Observer* observer)
    {
        for (auto& obs : _observers)
        {
            if (obs.second.contains(observer))
                obs.second.remove(observer);
        }
    }

    void Observed::notify(const std::string& type, const std::any& data, Observer* src)
    {
        lg("Observed::notify " + type);
        auto& obs = _observers[type];
        lg("Find " + std::to_string(obs.size()) + " observers");
        for (auto observer : obs)
        {
            if (observer != src)
            {
                lg("Observer is not the same as source, so we update it...");
                observer->update(type, data);
            }
        }
    }
}
