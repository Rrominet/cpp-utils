#pragma once
#include "../ObjectsManager.h"
#include "./Entity.h"
#include "../vec.h"

namespace ml
{
    namespace nodes
    {
        class Graph : public Entity
        {
            public :
                Graph(Workflow* workflow) : Entity(workflow) {}

            private: 
                ml::Vec<Handle<Node>> _nodes;
                ml::Vec<Handle<Link>> _links;
        };
    }
}
