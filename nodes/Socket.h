#pragma once
#include "../tconv.h"
#include "./Entity.h"
#include "../ObjectsManager.h"
#include "./enums.h"

namespace ml
{
    namespace nodes 
    {
        class Node;
        class Link;
        class BaseSocket : public ml::nodes::Entity
        {
            public : 
                BaseSocket(Workflow* workflow, Handle<Node> node, SocketType type, const std::string& name) : Entity(workflow), _node(node), _type(type), _name(name) {}
                virtual ~BaseSocket() = default;
                bool hasError() const;
                std::string typeError() const;
                std::string errors() const;

                bool connected() const {return _link.valid();}
                Link* link() {return _link.get();}
                Node* node() {return _node.get();}
                BaseSocket* connectedSocket();

                void setLink(Handle<Link> link) {_link = link;}

                std::string name() const {return _name;}

                template<typename T>
                T defaultValue() const
                {
                    return tconv::converted<T>(_defaultValue);
                }

                template<typename T>
                ml::Vec<T> value() 
                {
                    lg("Getting value for " << _name);
                    _typeError = "";
                    ml::Vec<T> vec;
                    if (_type == SocketType::OUTPUT)
                    {
                        lg("It's an output, so getting the _value");
                        vec.resize(_value.size());
                        for (size_t i=0; i<_value.size(); i++)
                            vec[i] = tconv::converted<T>(_value[i]);
                        return vec;
                    }

                    auto csocket = this->connectedSocket();
                    if (!csocket && _type == SocketType::INPUT)
                    {
                        lg("It's an input and the socket is not connected. Getting the default value.");
                        vec.push_back(this->defaultValue<T>());
                    }
                    else
                    {
                        lg("It's an input and the socket is connected. Getting the of the connected output.");
                        lg("Connected socket : " << csocket->name());
                        ml::Vec<T> connectedValue = csocket->value<T>();
                        vec.resize(connectedValue.size());
                        for (size_t i=0; i<vec.size(); i++)
                            vec[i] = connectedValue[i];
                    }
                    return vec;
                }

                virtual json serialize()override;
                virtual void deserialize(const json& j) override;

            protected : 
                // _defaultValue is the default value setted by the user (often gui) for any input
                std::any _defaultValue;

                //the _value is the actual value of the socket compiled by his node (output setted by the node, input setted by its connected output)
                ml::Vec<std::any> _value;

                std::string _typeError;
                std::string _name;

                Handle<Link> _link;
                Handle<Node> _node;
                SocketType _type = NONE;
        };


        template<typename T>
            class Socket : public ml::nodes::BaseSocket
        {
            public : 
                Socket(Workflow* workflow, Handle<Node> node, SocketType type, const std::string& name) : ml::nodes::BaseSocket(workflow, node, type, name)
                {
                    tconv::setAsDefault(_defaultValue);
                }
                virtual ~Socket() = default;

                template<typename Tin>
                    void setDefaultValue(const Tin value)
                    {
                        _set<Tin>(value, &_defaultValue);
                    }

                template<typename Tin>
                    void setUniqueValue(const Tin value)
                    {
                        if (_value.empty())
                            _value.resize(1);

                        for (auto& v : _value)
                            _set<Tin>(value, &v);
                    }

                template<typename Tin>
                    void set(const std::vector<Tin>& value)
                    {
                        lg("Setting value (size = " << value.size() << ")");
                        _value.resize(value.size()); 
                        for (size_t i=0; i<value.size(); i++)
                            _set<Tin>(value[i], &_value[i]);
                    }

                //TODO : need to have a way to manage types that are not managed by json by default.
                virtual json serialize()override
                {
                    json j = ml::nodes::BaseSocket::serialize(); 
                    j["defaultValue"] = tconv::converted<T>(_defaultValue);
                    return j;
                }

                virtual void deserialize(const json& j) override 
                {
                    ml::nodes::BaseSocket::deserialize(j);
                    if (j.contains("defaultValue"))
                        _set<T>(j["defaultValue"].get<T>(), &_defaultValue);
                }


            protected : 
                template<typename Tin>
                void _set(const Tin value, std::any* var)
                {
                    lg("Setting value " << value << " in " << var);
                    T val = tconv::convert<Tin, T>(value, &_typeError);
                    lg("Converted value = " << val);
                    lg("Type error = " << _typeError);
                    *var = val;
                }
        };
    }
}
