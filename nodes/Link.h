#pragma once
#include "../ObjectsManager.h"
#include "./Entity.h"

namespace ml
{
    namespace nodes
    {
        class BaseSocket;
        class Graph;
        class Link : public Entity
        {
            public : 
                Link(Workflow* workflow, Handle<Graph> graph) : Entity(workflow), _graph(graph){}
                BaseSocket* socketIn() {return _socketIn.get();};
                BaseSocket* socketOut() {return _socketOut.get();};

                void setSocketOut(const Handle<BaseSocket>& socket){ _socketOut = socket;}
                void setSocketIn(const Handle<BaseSocket>& socket){ _socketIn = socket;}

                virtual json serialize()override;
                virtual void log() override;

            protected : 
                Handle<BaseSocket> _socketOut;
                Handle<BaseSocket> _socketIn;
                Handle<Graph> _graph;
        };
    }
}
