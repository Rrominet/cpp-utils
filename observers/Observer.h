#pragma once
#include <any>
#include <string>
#include <unordered_map>
#include <functional>
#include "vec.h"

/*
   How does this work : 
   Really simple, imagine you have a class that contains some data, it will be your Observed class (it should inherits from Observed)
   Now anytime, you change data in the observed class, you simply call Observed::notify(type, data_updated)

   Secondly, you need to create an observer class (it should inherits from Observer, OR the class could just contains it, the 2 systems work)
   the observer is typically a class(or its container class, or anything really) that need to be updated if the Observed class changes its data.

   When Observed calls notify(type, data) it will call the observer update(type, data) method automatically (you could to reimplement it but its default behavior should be good enough.)

   this update default implementation call the functions in _onUpdate[type] in the order that they has been added.

Note : 
the notify method contains a src argument that could be used to trigger event from an oberver to the observed data without creating a inifinte update loop

For this just call _observed->notify(type, data, this) in one of the Observer methods.

Important note : with this system, an Observed can have multiple observers. But an observer observe one Observed. (if you don't respect that, you will have dangling pointers everywhere and will crash when deleting the observed objects.)

*/

namespace ml
{
    class Observed;
    class Observer    
    {
        public:
            Observer() = default;
            virtual ~Observer();

            //this is trigger by the Observed object when their value (or anything really) changed.
            //the type is the type of the data changed (settings ? options ? description ? ...) 
            //the data is the data itself (that could be empty = 0)
            //you could reimplement this to handle the updates you want to do when the data of the observed object changes
            //but you have a basic default implementation that will just call all the functions in _onUpdate (this is a large part of the time sufficient)
            virtual void update(const std::string& type, const std::any& data);

            // you should put true to the second arg if you don't want to call _observed->addObserver(this, type);
            // its default by default for historical reasons
            // if observed is not null, it will call observed->addObserver(this, type); (that in intern make _observed point to observed), this is typically what you want unless you already called it.
            void addOnUpdate(const std::string& type, const std::function<void(const std::any&)>& func, Observed* observed=nullptr);
            void removeOnUpdate(const std::string& type);

        protected:
                std::unordered_map<std::string, 
                    ml::Vec<std::function<void(const std::any&)>>> _onUpdate;
            
        private:
            friend class Observed;
            Observed* _observed = nullptr;
    };
}
