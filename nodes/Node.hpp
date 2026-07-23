#pragma once
#include "./Node.h"
#include "./Workflow.h"

namespace ml
{
    namespace nodes
    {
        template<typename T>
            Handle<Socket<T>> Node::_createSocket(const std::string& name, SocketType type, const T& defaultValue )
            {
                lg("Node::_createSocket(" << name << ")");
                auto selfh = _workflow->manager().handle<Node>(this);
                if (!selfh.valid())
                {
                    lg("selfh is not valid, returning a null handle<Socket>");
                    return Handle<Socket<T>>();
                }
                auto hd = _workflow->manager().create<Socket<T>>(_workflow, selfh, type, name);
                if (auto* s = hd.get())
                {
                    s->setDefaultValue(defaultValue); 
                    if (type == SocketType::INPUT)
                        _socketsIn.push_back(hd);
                    else if (type == SocketType::OUTPUT)
                        _socketsOut.push_back(hd);
                }
                lg("Created socket " << name);
                hd.log();
                return hd;
            }

    }
}

