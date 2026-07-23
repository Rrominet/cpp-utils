#include "./Graph.h"
#include "./Workflow.h"

//should be gnerated at compile time
#include "sub/MathNode.h"

#include "./Graph.hpp"

namespace ml
{
    namespace nodes
    {
        template<>
            Node* Graph::fromId<Node>(const std::string& id)
            {
                lg("Graph::fromId<Node>(" << id << ")");
                for (auto& nh : _nodes)
                {
                    if (auto n = nh.get())
                    {
                        if (n->id() == id)
                            return n;
                    }
                }
                return nullptr;
            }

        template<>
            BaseSocket* Graph::fromId<BaseSocket>(const std::string& id)
            {
                lg("Graph::fromId<BaseSocket>(" << id << ")");
                for (auto& nh : _nodes)
                {
                    if (auto n = nh.get())
                    {
                        for (auto& s : n->inputs())
                        {
                            if (s->id() == id)
                                return s;
                        }
                        for (auto& s : n->outputs())
                        {
                            if (s->id() == id)
                                return s;
                        }
                    }
                }
                return nullptr;
            }

        template<>
            Link* Graph::fromId<Link>(const std::string& id)
            {
                lg("Graph::fromId<Link>(" << id << ")");
                for (auto& lh : _links)
                {
                    if (auto l = lh.get())
                    {
                        if (l->id() == id)
                            return l;
                    }
                }
                return nullptr;
            }

        Handle<Link> Graph::connect(Handle<BaseSocket> socketOut,Handle<BaseSocket> socketIn)
        {
            //TODO : check for multiple input links.
            lg("Graph::connect");
            auto hd = _workflow->manager().create<Link>(_workflow, _workflow->manager().handle<Graph>(this));
            if (auto* l = hd.get())
            {
                bool inok = false, outok = false;;
                if (auto s = socketOut.get())
                {
                    s->setLink(hd);
                    outok = true;
                }
                else 
                    lg("socketOut is null");
                if (auto s = socketIn.get())
                {
                    s->setLink(hd);
                    inok = true;
                }
                else 
                    lg("socketIn is null");

                if (inok && outok)
                {
                    l->setSocketOut(socketOut);
                    l->setSocketIn(socketIn);
                    _links.push_back(hd);
                }
            }
            return hd;
        }

        void Graph::execute()
        {
            _computeEngine.compute(_depEngine.executionLists(this));
        }

        json Graph::serialize()
        {
            auto j = Entity::serialize();
            j["name"] = _name;
            j["nodes"] = json::array();
            for (auto& nh : _nodes)
            {
                if (auto n = nh.get())
                    j["nodes"].push_back(n->serialize());
            }

            for (auto& lh : _links)
            {
                if (auto l = lh.get())
                    j["links"].push_back(l->serialize());
            }

            return j;
        }

        void Graph::deserialize(const json& j)
        {
            Entity::deserialize(j);
            _nodes.clear();
            _links.clear();

            if (j.contains("name"))
                _name = j["name"].get<std::string>();

            for (const auto& nj : j["nodes"])
            {
                //TODO should generate an error if type is not valid type
                std::string type = nj.value("type", "Node");
                        
                //should be generated at compile time
                if (type == "Node")
                {
                    auto hd = this->createNode<Node>("");
                    if (auto* n = hd.get())
                        n->deserialize(nj);
                }

                else if (type == "MathNode")
                {
                    auto hd = this->createNode<MathNode>("");
                    if (auto* n = hd.get())
                        n->deserialize(nj);
                }
                //
            }

            lg("Graph " << _name << " node deserialized -- _nodes.size() = " << _nodes.size());

            for (const auto& lj : j["links"])
            {
                if (!lj.contains("output"))
                    continue;
                if (!lj.contains("input"))
                    continue;

                BaseSocket* sout = this->fromId<BaseSocket>(lj["output"]);
                if (!sout)
                {
                    lg("socket out " << lj["output"].get<std::string>() << " not found in the graph " << _name);
                    continue;
                }

                BaseSocket* sin = this->fromId<BaseSocket>(lj["input"]);
                if (!sin)
                {
                    lg("socket in " << lj["input"].get<std::string>() << " not found in the graph " << _name);
                    continue;
                }

                auto hsout = _workflow->manager().handle<BaseSocket>(sout);
                hsout.log();
                if (!hsout.valid())
                {
                    lg("socket out " << sout->id() << " found in graph but not in object manager.");
                    continue;
                }
                auto hsin = _workflow->manager().handle<BaseSocket>(sin);
                hsin.log();
                if (!hsin.valid())
                {
                    lg("socket in " << sin->id() << " found in graph but not in object manager.");
                    continue;
                }

                this->connect(hsout, hsin);
            }

            lg("Graph " << _name << " link deserialized -- _links.size() = " << _links.size());
        }


        void Graph::log()
        {
            Entity::log();            
            lg("Graph " << _name << " :");
            for (auto& nh : _nodes)
            {
                if (auto n = nh.get())
                    n->log();
            }
            for (auto& lh : _links)
            {
                if (auto l = lh.get())
                    l->log();
            }
        }
    }
}

