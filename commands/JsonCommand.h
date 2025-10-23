#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Command.h"
#include "vec.h"


class JsonCommand : public ml::Command
{
    public:
        JsonCommand() = default;
        virtual ~JsonCommand() = default;

        virtual bool check() override;
        void setJsonArgs(const json& j);

        void setJsonExec(const std::function<void(const json&)>& f);
        void setJsonReverse(const std::function<void(const json&)>& f);

        std::string returnString(){return _returnValue.dump();}

        virtual json serialize() const;
        virtual void deserialize(const json& j);

    private : 
        ml::Vec<std::string> _mendatoryKeys; //bp cgs
        ml::Vec<std::function<bool(const std::string&)>> _mendatoryChecks; //bp cgs
        
        // equivalent of the _returned attr of Command, just keeping it for compatibility issues, whould use _returned in modern code. (unless you want to use the returnString() method.)
        json _returnValue; //bp cgs

    public : 
#include "./JsonCommand_gen.h"
};
