#pragma once
#include "./Link.h"
#include "./Socket.h"
#include "./Graph.h"
#include "./Workflow.h"

namespace ml
{
    namespace nodes
    {
        json Link::serialize()
        {
            json j = Entity::serialize();
            if (auto s = _socketOut.get())
                j["output"] = s->id();
            if (auto s = _socketIn.get())
                j["input"] = s->id();
            return j;
        }

        void Link::deserialize(const json& j)
        {
            Entity::deserialize(j);

            lg("Link::deserialize()");
            auto* graph = _graph.get();
            if (!graph)
            {
                lg("Graph is nullptr, should not happen.");
                return;
            }

            if (j.contains("output"))
            {
                auto sid = j["output"].get<std::string>();
                auto socket = graph->fromId<BaseSocket>(sid);
                if (socket)
                    _socketOut = _workflow->manager().handle<BaseSocket>(socket);
                else
                    lg("Output socket " << sid << " not found in the graph " << graph->id());
            }

            if (j.contains("input"))
            {
                auto sid = j["input"].get<std::string>();
                auto socket = graph->fromId<BaseSocket>(sid);
                if (socket)
                    _socketIn = _workflow->manager().handle<BaseSocket>(socket);
                else
                    lg("Input socket " << sid << " not found in the graph " << graph->id());
            }
        }
    }
}
