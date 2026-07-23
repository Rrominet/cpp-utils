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
            lg("Graph::connect");
            auto hd = _workflow->manager().create<Link>(_workflow, _workflow->manager().handle<Graph>(this));
            if (auto* l = hd.get())
            {
                l->connect(socketOut, socketIn);

                if (auto s = socketIn.get())
                    s->setLink(hd);
                else 
                    lg("socketIn is null");
                if (auto s = socketOut.get())
                    s->setLink(hd);
                else 
                    lg("socketOut is null");

                _links.push_back(hd);
            }
            return hd;
        }

        Handle<Link> Graph::createLinkFromData(const json& data)
        {
            auto hd = _workflow->manager().create<Link>(_workflow, _workflow->manager().handle<Graph>(this));
            if (auto* l = hd.get())
            {
                l->deserialize(data);

                Handle<BaseSocket> hsout, hsin;
                if (data.contains("output"))
                {
                    auto sid = data["output"].get<std::string>();
                    auto socket = this->fromId<BaseSocket>(sid);
                    if (socket)
                    {
                        socket->setLink(hd);
                        hsout = _workflow->manager().handle<BaseSocket>(socket);
                    }
                }

                if (data.contains("input"))
                {
                    auto sid = data["input"].get<std::string>();
                    auto socket = this->fromId<BaseSocket>(sid);
                    if (socket)
                    {
                        socket->setLink(hd);
                        hsin = _workflow->manager().handle<BaseSocket>(socket);
                    }
                }

                if (hsout.valid() && hsin.valid())
                {
                    l->connect(hsout, hsin);
                    _links.push_back(hd);
                }
            }
            return hd;
        }

        void Graph::execute()
        {
            auto lists = this->executionLists();
            for (auto& l : lists)
                this->computeList(l);
        }

        ml::Vec<ml::Vec<Node*>> Graph::executionLists()
        {
            ml::Vec<ml::Vec<Node*>> r;
            auto lasts = this->lasts();
            for (auto l : lasts)
            {
                _visited.clear();
                r.push_back(this->executionList(l));
            }

            return r;
        }

        ml::Vec<Node*> Graph::executionList(Node* node)
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

        void Graph::computeList(const ml::Vec<Node*>& list)
        {
            for (int64_t i = list.size() - 1; i >= 0; i--)
                list[i]->__execute__();
        }

        ml::Vec<Node*> Graph::lasts()
        {
            auto nodes = ml::managed::fromVector<Node>(&_workflow->manager(), _nodes);          
            ml::Vec<Node*> lasts;
            for (auto& n : nodes)
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
                //TODO should generate an error if type does not exists or is not valid type
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
                this->createLinkFromData(lj);

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

