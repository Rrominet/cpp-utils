#include "./Command.h"

namespace ml
{
    void Command::exec()
    {
        if (!this->check())
            throw std::runtime_error("Command check failed : " + this->name() + " : " + _error);
        if (_exec)
            _exec(_args);
    }

    void Command::reverse()
    {
        if (!this->checkReverse())
            throw std::runtime_error("Command reverse check failed : " + this->name() + " : " + _error);
        if (_reverse)
            _reverse(_args);
    }

    json Command::serialize() const
    {
        json _r;
        _r["type"] = "command";
        _r["id"] = _id;
        _r["name"] = _name;
            _r["help"] = _help;


        _r["aliases"] = json::array();
        for (const auto& alias : _aliases)
            _r["aliases"].push_back(alias);

        _r["userData"] = _userData;

        return _r;
    }

    void Command::deserialize(const json& j)
    {
        if (j.contains("id"))
            _id = j["id"];

        if (j.contains("name"))
            _name = j["name"];

        if (j.contains("help"))
            _help = j["help"];

        if (j.contains("keybind"))
            _keybind = j["keybind"];

        if (j.contains("aliases"))
        {
            _aliases.clear();
            for (const auto& alias : j["aliases"])
                _aliases.push_back(alias);
        }

        if (j.contains("userData"))
            _userData = j["userData"];
    }
}
