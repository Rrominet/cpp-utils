#pragma once
#include <mutex>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ml
{
    class DataBaseObject
    {
        public:
            DataBaseObject() = default;
            virtual ~DataBaseObject() = default;
            
            virtual std::string filepath() = 0;
            virtual json serialize() const = 0;
            virtual void deserialize(const json& data) = 0;

            // as a user you could overload it but keep in mine that you should called serialize in it in a threadsafe manner.
            // the default implementation just do that.
            virtual std::string stringifyied();

            virtual bool isList(){return false;}
            virtual long size() const = 0;

        protected : 
            std::mutex _mtx;
    };
}
