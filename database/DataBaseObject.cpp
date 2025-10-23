#include "./DataBaseObject.h"
#include <mutex>

namespace ml
{
    std::string DataBaseObject::stringifyied()
    {
        json data;
        {
            std::lock_guard<std::mutex> lk(_mtx);
            data = this->serialize();
        }

        return data.dump();
    }
}

