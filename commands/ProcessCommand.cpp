#include "./ProcessCommand.h"
#include "../mlprocess.h"
#include <boost/process.hpp>
#include "str.h"

namespace ml
{
    void ProcessCommand::exec()
    {
        this->convertFullCommandToArgs();
        lg("ProcessCommand::exec");
        if (!this->check())
            throw std::runtime_error("Command check failed : " + this->name() + " : " + _error);

        if (_detached)
        {
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
        else 
        {
            auto pc = _processPath;
            auto args = _processArgs;
            bp::ipstream pipe_out, pipe_err;

            bp::child c(
                    _processPath,
                    bp::args(_processArgs.vec),
                    bp::std_out > pipe_out,
                    bp::std_err > pipe_err
            );
            std::ostringstream ss_out, ss_err;
            c.wait();
            std::string line;
            while (std::getline(pipe_out, line)) ss_out << line << "\n";
            while (std::getline(pipe_err, line)) ss_err << line << "\n";

            _stdout = ss_out.str();
            _stderr = ss_err.str();
            _exitCode = c.exit_code();
        }
    }

    json ProcessCommand::serialize() const
    {
        json _r = Command::serialize();
        _r["type"] = "processCommand";
        _r["processPath"] = _processPath;
        _r["processArgs"] = json::array();
        for (const auto& arg : _processArgs)
            _r["processArgs"].push_back(arg);
        _r["detached"] = _detached;
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
        if (j.contains("detached"))
            _detached = j["detached"];
    }

    void ProcessCommand::convertFullCommandToArgs()
    {
        if (!str::contains(_processPath, " "))
            return;

        ml::Vec<std::string> args;

        args = process::parse(_processPath);
        if (args.empty())
            return;
        _processPath = args[0];
        args.vec.erase(args.vec.begin());
        args.concat(_processArgs.vec);
        _processArgs = args;
    }
}
