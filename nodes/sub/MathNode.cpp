#include "./MathNode.h"

#include "../Graph.hpp"
#include "../Node.hpp"

namespace ml
{
    namespace nodes
    {
        MathNode::MathNode(Workflow* workflow, Handle<Graph> graph, const std::string& name) : Node(workflow, graph, name)
        {
            lg("MathNode::MathNode(" << name << ")");
            _init.push_back([](Node* self) {static_cast<MathNode*>(self)->init();});
        };

        void MathNode::init()
        {
            lg("MathNode::init");
            this->createSockets();
            _exec.push_back([](Node* self) {static_cast<MathNode*>(self)->exec();});
        }

        void MathNode::createSockets()
        {
            lg("MathNode::createSockets");
            _a = this->createInput<double>("A");
            _b = this->createInput<double>("B");
            _c = this->createOutput<double>("C");
        }

        void MathNode::exec()
        {
            lg("MathNode::exec() -- " + _name) ;
            auto sa = _a.get();  
            auto sb = _b.get();  
            auto sc = _c.get();  

            if (sa && sb && sc)
            {
                auto sav = sa->value<double>();
                auto sbv = sb->value<double>();
                auto scv = sc->value<double>();
                this->exec_list(sav,sbv,scv);

                lg("C socket value after exec = " << scv[0]);
                sc->set<double>(scv);
            }
        }

        void MathNode::exec_list(ml::Vec<double>& a,ml::Vec<double>& b,ml::Vec<double>& c)
        {
            lg("MathNode::exec_list() -- " + _name) ;
            if (a.size() < b.size()) 
                vc::extendWithLastValue<double>(a, b.size());

            if (b.size() < a.size()) 
                vc::extendWithLastValue<double>(b, a.size());

            c.resize(a.size());
            for (size_t i=0; i<a.size(); i++)
                this->exec_one(a[i],b[i],c[i]);
        }

        json MathNode::serialize()
        {
            lg("MathNode::serialize");
            json j = Node::serialize();
            if (auto sa = _a.get())
                j["in_a"] = sa->serialize();
            if (auto sb = _b.get())
                j["in_b"] = sb->serialize();
            if (auto sc = _c.get())
                j["out_c"] = sc->serialize();

            return j;
        }

        void MathNode::deserialize(const json& j)
        {
            lg("MathNode::deserialize");
            Node::deserialize(j);
            if (j.contains("in_a"))
                _a.get()->deserialize(j["in_a"]);
            if (j.contains("in_b"))
                _b.get()->deserialize(j["in_b"]);
            if (j.contains("out_c"))
                _c.get()->deserialize(j["out_c"]);
        }

        void MathNode::exec_one(double a,double b,double& c)
        {
            lg("MathNode::exec_one() -- " + _name) ;
            lg("a = " << a << " b = " << b);
            c = a + b;        
            lg("c = " << c);
        }

        void MathNode::log()
        {
            Node::log();
            lg("MathNode " << _name << " (c = a + b)");
            auto val = _a.get()->value<double>();
            if (val.size() > 0)
                lg("Output (C) value = " << val[0]);
        }
    }
}

