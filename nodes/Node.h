#pragma once
#include "./Workflow.h"
#include "./Socket.h"
#include "../vec.h"

namespace ml
{
    namespace nodes
    {
        class Node : public Entity
        {
            public : 
                enum SocketType
                {
                    NONE = 0,
                    INPUT,
                    OUTPUT
                };

                template<typename T>
                    Handle<Socket<T>> createInput(const std::string& name, const T& defaultValue = T()){return _createSocket<T>(name, SocketType::INPUT, defaultValue);}
                template<typename T>
                    Handle<Socket<T>> createOutput(const std::string& name, const T& defaultValue = T()){return _createSocket<T>(name, SocketType::OUTPUT, defaultValue);}

            protected: 
                ml::Vec<Handle<BaseSocket>> _socketsIn;
                ml::Vec<Handle<BaseSocket>> _socketsOut;

                template<typename T>
                    Handle<Socket<T>> _createSocket(const std::string& name, SocketType type, const T& defaultValue = T())
                    {
                        auto hd = _workflow->manager().create<Socket<T>>(name);
                        if (auto* s = hd.get())
                        {
                            s->setDefaultValue(defaultValue); 
                            if (type == SocketType::INPUT)
                                _socketsIn.push_back(hd);
                            else if (type == SocketType::OUTPUT)
                                _socketsOut.push_back(hd);
                        }
                        return hd;
                    }
        };
    }
}


