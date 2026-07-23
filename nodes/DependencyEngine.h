#pragma once
#include "../vec.h"

namespace ml
{
    namespace nodes
    {
        class Workflow;
        class Graph;
        class Node;
        class DependencyEngine
        {
            public : 
                DependencyEngine(Workflow* workflow) : _workflow(workflow){}
                ml::Vec<ml::Vec<Node*>> executionLists(Graph* graph);
                ml::Vec<Node*> executionList(Node* node);
                ml::Vec<Node*> lasts(Graph* graph);
            
            private : 
                ml::Vec<Node*> _visited;
                Workflow* _workflow = nullptr;
        };
    }
}
