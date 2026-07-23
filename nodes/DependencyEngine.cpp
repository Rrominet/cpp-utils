#include "./DependencyEngine.h"
#include "./Workflow.h"
#include "./Graph.h"
#include "./Node.h"

namespace ml
{
    namespace nodes
    {
        ml::Vec<ml::Vec<Node*>> DependencyEngine::executionLists(Graph* graph)
        {
            ml::Vec<ml::Vec<Node*>> r;
            auto lasts = this->lasts(graph);
            for (auto l : lasts)
            {
                _visited.clear();
                r.push_back(this->executionList(l));
            }

            return r;
        }

        ml::Vec<Node*> DependencyEngine::executionList(Node* node)
        {
            ml::Vec<Node*> list;
            list.push_back(node);
            _visited.push_back(node);
            for (auto i : node->inputs()) 
            {
                if (i->connected())
                {
                    auto nnode = i->connectedSocket()->node();
                    if (!_visited.contains(nnode))
                        list.concat(this->executionList(nnode));
                }
            }
            return list;
        }

        ml::Vec<Node*> DependencyEngine::lasts(Graph* graph)
        {
            auto nodes = ml::managed::fromVector<Node>(&_workflow->manager(), graph->nodes());          
            ml::Vec<Node*> lasts;
            for (const auto& n : nodes)
            {
                if (n->outputs().size() == 0)
                {
                    lasts.push_back(n);
                    continue;
                }

                bool out_connected = false;
                for (auto& s : n->outputs())
                {
                    if (s->connected())
                    {
                        out_connected = true;
                        break;
                    }
                }
                if (!out_connected)
                    lasts.push_back(n);
            }
            return lasts;
        }
    }
}
