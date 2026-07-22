#pragma once
#include "./Socket.h"
#include "../vec.h"
#include "./enums.h"
#include <functional>

namespace ml
{
    namespace nodes
    {
        class Workflow;
        class Graph;
        class Node : public Entity
        {
            public : 
                Node(Workflow* workflow, Handle<Graph> graph, const std::string& name) : Entity(workflow), _graph(graph), _name(name) {}

                template<typename T>
                    Handle<Socket<T>> createInput(const std::string& name, const T& defaultValue = T()){return _createSocket<T>(name, SocketType::INPUT, defaultValue);}
                template<typename T>
                    Handle<Socket<T>> createOutput(const std::string& name, const T& defaultValue = T()){return _createSocket<T>(name, SocketType::OUTPUT, defaultValue);}

                ml::Vec<BaseSocket*> inputs();
                ml::Vec<BaseSocket*> outputs();

                //this syntax is to be sure a child class don't come shadow it.
                void __execute__();

                //executed by the ObjectManager just after construction
                //PUT Everything you need to be initialized here NOT in the Constructor (in practise add functions in _init)
                void __init__();

                virtual json serialize()override;
                virtual void deserialize(const json& j) override;

            protected: 
                std::string _name;
                std::string _type = "Node";
                Handle<Graph> _graph;

                ml::Vec<Handle<BaseSocket>> _socketsIn;
                ml::Vec<Handle<BaseSocket>> _socketsOut;

                ml::Vec<std::function<void(Node* self)>> _exec;

                //add your init functions here
                ml::Vec<std::function<void(Node* self)>> _init;

                template<typename T>
                    Handle<Socket<T>> _createSocket(const std::string& name, SocketType type, const T& defaultValue = T());

        };
    }
}


