#pragma once
#include "../ObjectsManager.h"
#include "../vec.h"

#include "./Entity.h"
#include "./Node.h"
#include "./Link.h"

#include "./DependencyEngine.h"
#include "./ComputeEngine.h"

namespace ml
{
    namespace nodes
    {
        class Graph : public Entity
        {
            public :
                Graph(Workflow* workflow, const std::string& name="") : Entity(workflow), 
                    _depEngine(workflow),
                    _computeEngine(workflow),
                    _name(name) {}
                
                template<typename N>
                    Handle<N> createNode(const std::string& name);

                Handle<Link> connect(Handle<BaseSocket> socketOut, Handle<BaseSocket> socketIn);

                virtual json serialize()override;
                virtual void deserialize(const json& j) override;

                template<typename N>
                    N* fromId(const std::string& id);

                const std::string& name() const {return _name;}
                virtual void log() override;

                const ml::Vec<Handle<Node>>& nodes()const {return _nodes;}
                ml::Vec<Handle<Node>>& nodes(){return _nodes;}

                const ml::Vec<Handle<Link>>& links()const {return _links;}
                ml::Vec<Handle<Link>>& links(){return _links;}

                void execute();

            private: 
                std::string _name;
                ml::Vec<Handle<Node>> _nodes;
                ml::Vec<Handle<Link>> _links;

                DependencyEngine _depEngine;
                ComputeEngine _computeEngine;
        };
    }
}
