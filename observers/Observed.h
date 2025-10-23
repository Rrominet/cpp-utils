#pragma once
#include "vec.h"
#include <any>
#include <unordered_map>

/*
   See Observer.h for the full description and how to implement this pattern (Observed and Observer)
 */

namespace ml
{
    class Observer;
    class Observed
    {
        public : 
            Observed() = default;
            virtual ~Observed();

            void addObserver(Observer* observer, const std::string& type="");
            void removeObserver(Observer* observer, const std::string& type="");
            void removeObserverFromAll(Observer* observer);
            void clearObservers(){_observers.clear();}

            // data will be send to all observers.
            void notify(const std::string& type="", const std::any& data=0, Observer* src=nullptr);

        protected : 
            std::unordered_map<std::string, ml::Vec<Observer*>> _observers;
    };
}
