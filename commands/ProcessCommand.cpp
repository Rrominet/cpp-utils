#include "./ProcessCommand.h"
#include "../mlprocess.h"

namespace ml
{
    void ProcessCommand::exec()
    {
        lg("ProcessCommand::exec");
        if (!this->check())
            throw std::runtime_error("Command check failed : " + this->name() + " : " + _error);

        //For now always in background, no possibility for the user to get the process stdout
        //We'll see if it become a problem later...
        std::vector<std::string> cmd = {_processPath};
        for (const auto& arg : _processArgs)
            cmd.push_back(arg);
        cmd.push_back("&");

        //this changed
        lg("Executing command via std::system : " << process::to_string(cmd));
        auto res = std::system(process::to_string(cmd).c_str());
        if (res != 0)
            throw std::runtime_error(_processPath + " : exec failed with code " + std::to_string(res));
    }

    json ProcessCommand::serialize() const
    {
        json _r = Command::serialize();
        _r["type"] = "processCommand";
        _r["processPath"] = _processPath;
        _r["processArgs"] = json::array();
        for (const auto& arg : _processArgs)
            _r["processArgs"].push_back(arg);
        return _r;
    }

    void ProcessCommand::deserialize(const json& j)
    {
        Command::deserialize(j);
        if (j.contains("processPath"))
            _processPath = j["processPath"];
        if (j.contains("processArgs"))
        {
            _processArgs.clear();
            for (const auto& arg : j["processArgs"])
                _processArgs.push_back(arg);
        }
    }
}
