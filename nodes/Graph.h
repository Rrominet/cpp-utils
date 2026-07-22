#pragma once
#include "../ObjectsManager.h"
#include "../vec.h"

#include "./Entity.h"
#include "./Node.h"
#include "./Link.h"

namespace ml
{
    namespace nodes
    {
        class Graph : public Entity
        {
            public :
                Graph(Workflow* workflow, const std::string& name) : Entity(workflow), _name(name) {}
                
                template<typename N>
                    Handle<N> createNode(const std::string& name);

                Handle<Link> connect(Handle<BaseSocket> socketOut, Handle<BaseSocket> socketIn);

                void execute();

                ml::Vec<ml::Vec<Node*>> executionLists();
                ml::Vec<Node*> executionList(Node* node);
                void computeList(const ml::Vec<Node*>& list);

                ml::Vec<Node*> lasts();

                virtual json serialize()override;
                virtual void deserialize(const json& j) override;

                template<typename N>
                    N* fromId(const std::string& id);

            private: 
                std::string _name;
                ml::Vec<Handle<Node>> _nodes;
                ml::Vec<Handle<Link>> _links;

                ml::Vec<Node*> _visited;
        };
    }
}
