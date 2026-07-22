#pragma once
#include "../str.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

//base class for all stuff in a node graph or used by a node graph
//used for the ObjectsManager in the ml::nodes::Workflow

namespace ml
{
    namespace nodes
    {
        class Workflow;

        class Entity
        {
            public : 
                Entity(Workflow* workflow) : _workflow(workflow) {_id = str::random(10);}
                virtual ~Entity() = default;

                virtual json serialize() { return json(); }
                virtual void deserialize(const json& j) {if (j.contains("id")) _id = j["id"].get<std::string>();}

                std::string id() const {return _id;}

            protected :
                std::string _id;
                Workflow* _workflow;
        };
    }
}
