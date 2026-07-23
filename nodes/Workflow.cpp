#include "./Workflow.h"
#include "../files.2/files.h"

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
            //clear all previous memory
            _manager.clear();

            for (auto& g : j["graphs"])
            {
                auto hg = _manager.create<Graph>(this);
                if (auto* graph = hg.get())
                    graph->deserialize(g);
                _graphs.push_back(hg);
            }
        }

        ml::Ret<> Workflow::load(const std::string& path)
        {
            std::string data;
            try 
            {
                data = files::read(path);
            }
            catch(const std::exception& e)
            {
                return ml::ret::fail("Error reading the worflow grahs file : " + std::string(e.what()));
            }

            json jdata;
            try
            {
                jdata = json::parse(data);
            }
            catch(const std::exception& e)
            {
                return ml::ret::fail("Error parsing the workflow graphs file : " + std::string(e.what()) + "\n" + data);
            }

            this->deserialize(jdata);
            return ml::ret::ok();
        }

        Graph* Workflow::graph(unsigned int idx)
        {
            if(idx >= _graphs.size())
                return nullptr;
            return _graphs[idx].get();
        }

        Graph* Workflow::graph(const std::string& name)
        {
            for(auto& gh : _graphs)
            {
                if (auto* g = gh.get())
                {
                    if (g->name() == name)
                        return g;
                }
            }
            return nullptr;
        }

        void Workflow::log()
        {
            for (auto& gh : _graphs)             
            {
                if (auto* g = gh.get())
                    g->log();
            }
        }
    }

}
