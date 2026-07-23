#pragma once
#include "../ObjectsManager.h"
#include "./Graph.h"
#include "./Node.h"

namespace ml
{
    namespace nodes
    {
        class Node;
        class Workflow 
        {
            public :
                Workflow()
                {
                    _manager.registerInitFunc<Node>([](Node* n){n->__init__();});
                };
                const ObjectsManager& manager() const { return _manager; }
                ObjectsManager& manager() { return _manager; }

                Handle<Graph> createGraph(const std::string& name);

                json serialize();
                void deserialize(const json& j);

                ml::Ret<> load(const std::string& path);

                Graph* graph(unsigned int idx);
                Graph* graph(const std::string& name);

                void log();

            private: 
                ObjectsManager _manager;
                ml::Vec<Handle<Graph>> _graphs;
        };
    }
}
