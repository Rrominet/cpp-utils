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

        void Link::log()
        {
            Entity::log();
            lg("Link " << _id << " :");
            if (auto* s = _socketOut.get())
                lg("  from output socket " << s->id() << " (" << s->name() << ")");
            if (auto* s = _socketIn.get())
                lg("  to input socket " << s->id() << " (" << s->name() << ")");
        }
    }
}
