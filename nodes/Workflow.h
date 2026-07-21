#pragma once
#include "../ObjectsManager.h"

namespace ml
{
    namespace nodes
    {
        class Workflow 
        {
            public :
                const ObjectsManager& manager() const { return _manager; }
                ObjectsManager& manager() { return _manager; }

            private: 
                ObjectsManager _manager;
        };
    }
}
