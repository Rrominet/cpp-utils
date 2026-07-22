#include "./Workflow.h"

namespace ml
{
    namespace nodes
    {

        Handle<Graph> Workflow::createGraph(const std::string& name)
        {
            auto hg = _manager.create<Graph>(this, name); 
            _graphs.push_back(hg);
            return hg;
        }

        json Workflow::serialize()
        {
            json j;
            j["graphs"] = json::array();
            for (auto& gh : _graphs)
            {
                if (auto* g = gh.get())
                    j["graphs"].push_back(g->serialize());
            }
            return j;
        }

        void Workflow::deserialize(const json& j)
        {
            //TODO remove all handles in the manager, clear the graphs.
        }
    }
}
