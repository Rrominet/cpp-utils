#pragma once
#include "./Graph.h"
#include "./Workflow.h"
#include "./tpl_utils.h"

namespace ml
{
    namespace nodes
    {
        template<typename N>
            Handle<N> Graph::createNode(const std::string& name)
            {
                auto hd = _workflow->manager().create<N, Node>(_workflow, _workflow->manager().handle<Graph>(this), name);
                if (auto* n = hd.get())
                    _nodes.push_back(hd);
                return hd;
            }

        template<typename N>
            N* Graph::fromId(const std::string& id)
            {
                lg("Graph::fromId<" << typeid(N).name() << ">(" << id << ")");
                if constexpr(has_id<N>::value)
                {
                    for (const auto& s : _workflow->manager().slots())
                    {
                        if (typeid(std::shared_ptr<N>) != s.object.type())
                            continue;

                        auto ptr = std::any_cast<std::shared_ptr<N>>(s.object);
                        if (ptr->id() == id)
                            return ptr.get();
                    }
                }
                else
                    return nullptr;
            }
    }
}
