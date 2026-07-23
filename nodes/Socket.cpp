#include "./Socket.h"
#include "./Link.h"
#include "enums.h"

namespace ml
{
    namespace nodes 
    {
        bool BaseSocket::hasError() const
        {
            return !_typeError.empty();
        }

        std::string BaseSocket::typeError() const
        {
            return _typeError;
        }

        std::string BaseSocket::errors() const
        {
            return _typeError;
        }

        BaseSocket* BaseSocket::connectedSocket()
        {
            if (auto* l = _link.get())            
            {
                if (_type == SocketType::INPUT)
                    return l->socketOut();
                else 
                    return l->socketIn();
            }
            return nullptr;
        }

        json BaseSocket::serialize()
        {
            json j = Entity::serialize();
            j["name"] = _name;
            j["type"] = _type;
            return j;
        }

        void BaseSocket::deserialize(const json& j)
        {
            Entity::deserialize(j);
            if (j.contains("name"))
                _name = j["name"].get<std::string>();
            if (j.contains("type"))
                _type = j["type"].get<SocketType>();
        }

        void BaseSocket::log()
        {
            Entity::log();
            lg("Socket " << _name << " (type: " << _type << ") :");
            if (_link.valid())
                lg("  connected to link " << _link.get()->id());
            else
                lg("  not connected");
            if (hasError())
                lg("  error: " << _typeError);
        }
    }
}
