#pragma once

//base class for all stuff in a node graph or used by a node graph
//used for the ObjectsManager in the ml::nodes::Workflow

namespace ml
{
    namespace nodes
    {
        class Workflow;

        class Entity
        {
            public : 
                Entity(Workflow* workflow) : _workflow(workflow) {}
                virtual ~Entity() = default;

            protected :
                Workflow* _workflow;
        };
    }
}
