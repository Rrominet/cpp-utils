#include "./Node.h"
#include "./Workflow.h"

namespace ml
{
    namespace nodes
    {
        ml::Vec<BaseSocket*> Node::inputs()
        {
            return ml::managed::fromVector<BaseSocket>(&_workflow->manager(), _socketsIn);
        }

        ml::Vec<BaseSocket*> Node::outputs()
        {
            return ml::managed::fromVector<BaseSocket>(&_workflow->manager(), _socketsOut);
        }

        void Node::__execute__()
        {
            lg("Node::__execute__");
            lg("Exec count = " << _exec.size());
            for (const auto& e : _exec)            
                e(this);
        }

        void Node::__init__()
        {
            lg("Node::__init__");
            lg("Init count = " << _init.size());
            for (const auto& e : _init)            
                e(this);
        }

        json Node::serialize()
        {
            auto j = Entity::serialize(); 
            j["name"] = _name;
            j["type"] = _type;

            j["inputs"] = json::array();
            for (auto& sh : _socketsIn)
            {
                if (auto s = sh.get())
                    j["inputs"].push_back(s->serialize());
            }

            j["outputs"] = json::array();
            for (auto& sh : _socketsOut)
            {
                if (auto s = sh.get())
                    j["outputs"].push_back(s->serialize());
            }
            return j;
        }

        void Node::deserialize(const json& j)
        {
            Entity::deserialize(j); 
            if (j.contains("name"))
                _name = j["name"].get<std::string>();
            if (j.contains("type"))
                _type = j["type"].get<std::string>();

            //TODO
        }
    }
}
