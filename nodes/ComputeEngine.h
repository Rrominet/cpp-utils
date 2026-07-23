#pragma once
#include "../vec.h"

namespace ml
{
    namespace nodes
    {
        class Node;
        class Workflow;
        class ComputeEngine
        {
            public : 
                ComputeEngine(Workflow* workflow) : _workflow(workflow){}
                void computeList(const ml::Vec<Node*>& list);
                void compute(const ml::Vec<ml::Vec<Node*>>& execlist);

            private : 
                Workflow* _workflow = nullptr;
                ml::Vec<Node*> _computed;
        };
    }

}
