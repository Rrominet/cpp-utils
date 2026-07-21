#pragma once
#include "../ObjectsManager.h"
#include "./Entity.h"

namespace ml
{
    namespace nodes
    {
        class Workflow 
        {
            public :
                const ObjectsManager<Entity>& manager() const { return _manager; }
                ObjectsManager<Entity>& manager() { return _manager; }

            private: 
                ObjectsManager<Entity> _manager;
        };
    }
}
