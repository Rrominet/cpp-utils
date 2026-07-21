#pragma once
#include "../ObjectsManager.h"
#include "./Socket.h"
#include "../vec.h"

namespace ml
{
    namespace nodes
    {
        class Node : public Entity
        {
            public : 
                template<typename T>
                    Handle<Socket<T>> createInputSocket(const std::string& name, const T& defaultValue = T());

            protected: 
                ml::Vec<Handle<BaseSocket>> _socketsIn;
                ml::Vec<Handle<BaseSocket>> _socketsOut;
        };
    }
}


