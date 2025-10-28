#include "./ProcessCommand.h"
namespace ml
{
    void ProcessCommand::exec()
    {
        if (!this->check())
            throw std::runtime_error("Command check failed : " + this->name() + " : " + _error);

        //For now always in background, no possibility for the user to get the process stdout
        //We'll see if it become a problem later...
        _processPath += " &";

        auto res = std::system(_processPath.c_str());
        if (res != 0)
            throw std::runtime_error(_processPath + " : exec failed with code " + std::to_string(res));
    }

    json ProcessCommand::serialize() const
    {
        json _r = Command::serialize();
        _r["type"] = "processCommand";
        _r["processPath"] = _processPath;
        return _r;
    }

    void ProcessCommand::deserialize(const json& j)
    {
        Command::deserialize(j);
        if (j.contains("processPath"))
            _processPath = j["processPath"];
    }
}
