#include "./JsonCommand.h"

bool JsonCommand::check()
{
    json& args = this->args<json>(); 	
    for (const auto& key : _mendatoryKeys)
    {
        if (!args.contains(key))
        {
            _error = "Missing key : " + key;
            return false;
        }

        for (const auto& _check : _mendatoryChecks)
        {
            if (!_check(args[key]))
            {
                _error = "Wrong value for key : " + key;
                return false;
            }
        }
    }
    return true;
}

void JsonCommand::setJsonArgs(const json& j)
{
    this->setArgs(j);
}

void JsonCommand::setJsonExec(const std::function<void(const json&)>& f)
{
    auto exec = [this, f](const std::any& args)
    {
        json j = this->args<json>();
        f(j);
    };
    this->setExec(exec);
}

void JsonCommand::setJsonReverse(const std::function<void(const json&)>& f)
{
    // no need to implement it for now.
    assert(false && "setJsonReverse Not implemented yet.");
}

json JsonCommand::serialize() const
{
    json _r = Command::serialize();	
    _r["type"] = "jsonCommand";
    _r["mendatoryKeys"] = json::array();
    for (const auto& key : _mendatoryKeys)
        _r["mendatoryKeys"].push_back(key);
    return _r;
}

void JsonCommand::deserialize(const json& j)
{
    Command::deserialize(j);	
    if (j.contains("mendatoryKeys"))
    {
        _mendatoryKeys.clear();
        for (const auto& key : j["mendatoryKeys"])
            _mendatoryKeys.push_back(key);
    }
}
	

