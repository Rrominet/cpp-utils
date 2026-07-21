#pragma once
#include "../tconv.h"
#include "./Entity.h"

namespace ml
{
    namespace nodes 
    {
        class BaseSocket : public ml::nodes::Entity
        {
            public : 
                BaseSocket(Workflow* workflow, const std::string& name) : Entity(workflow), _name(name) {}
                virtual ~BaseSocket() = default;
                bool hasError() const;
                std::string typeError() const;
                std::string errors() const;

            protected : 
                std::any _defaultValue;
                std::string _typeError;
                std::string _name;
        };


        template<typename T>
            class Socket : public ml::nodes::BaseSocket
        {
            public : 
                Socket(Workflow* workflow, const std::string& name) : ml::nodes::BaseSocket(workflow, name)
                {
                    tconv::setAsDefault(_defaultValue);
                }
                virtual ~Socket() = default;

                template<typename Tin>
                    void setDefaultValue(const Tin value)
                    {
                        T val = tconv::convert<Tin, T>(value, &_typeError);
                        _defaultValue = std::any(val);
                    }

                T defaultValue() const
                {
                    return std::any_cast<T>(_defaultValue);
                }
        };
    }
}
