#pragma once
#include "../Node.h"
#include <functional>

namespace ml
{
    namespace nodes
    {
        class MathNode : public Node
        {
            public:
                MathNode(Workflow* workflow, const std::string& name);

                void createSockets();

                void exec();
                void exec_list(ml::Vec<double>& a, ml::Vec<double>& b, ml::Vec<double>& c);
                void exec_one(double a, double b, double& c);

                Handle<Socket<double>> a() { return _a; }
                Handle<Socket<double>> b() { return _b; }
                Handle<Socket<double>> c() { return _c; }

                //executed by the ObjectManager just after construction
                //PUT Everything you need to be initialized here NOT in the Constructor
                void init();

            protected : 
                Handle<Socket<double>> _a;
                Handle<Socket<double>> _b;
                Handle<Socket<double>> _c;
        };
    }
}
