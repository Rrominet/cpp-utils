#include "./Agent.h"
#include "../mlTime.h"
#include "../files.2/files.h"
#include "../ipc.h"

namespace ml 
{
    const std::string Agent::name() const
    {
        if(!_name.empty())
            return _name;
        return _id;
    }

    void Agent::_writeLog(const std::string& msg)
    {
        if (_config.logFile().empty())        
            return;
        std::string data;
        data = ml::time::asString(ml::time::now()) + ":\n" + msg;
        try
        {
            files::append(_config.logFile(), data);
        }
        catch(const std::exception& e)
        {
            lg("Agent Error while writing the log file : " << _config.logFile() << "\n\nMore Infos : \n" << e.what());
        }
    }

    void Agent::_writeWhatItSent()
    {
        std::string data = "Sending :";
        data += "\nContext : " + _context + "\n\n";
        data += "\nMessage : " + _inData;

        _writeLog(data);
    }

    void Agent::_writeWhatItReceived(const std::string& received)
    {
        std::string data = "Received :\n" + received;
        _writeLog(data);
    }

    Ret<> Agent::exec(Process* llm)
    {
        if (!llm)
            return ml::ret::fail("The llm process is null.");
        if (_inData.empty())
            return ml::ret::fail("The in data is empty. Nothing to send to the llm.");

        std::string _beforeRes;
        for (const auto& f : _beforeLlmCall)
        {
            auto res = f(_inData);
            _beforeRes += res + "\n";
        }

        if (!_beforeRes.empty())
        {
            _beforeRes.pop_back();
            _inData += "More information to help you complete your task : \n" + _beforeRes;
        }

        _writeWhatItSent();
        json pdata;
        pdata["context"] = _context;
        pdata["messages"] = json::array();
        pdata["messages"].push_back({{"role", "user"}, {"content", _inData}});
        pdata["model"] = _config.model();
        pdata["api-keys"] = _config.api_keys();
        pdata["max-tokens"] = _config.max_tokens();

        auto res = ipc::call_sync(llm, "send", pdata);
        if(!res.contains("success") || !res["success"])
        {
            if (res.contains("message"))
                return ml::ret::fail("Llm call failed :\n " + res["message"].get<std::string>());
            else if (res.contains("error"))
                return ml::ret::fail("Llm call failed :\n " + res["error"].get<std::string>());
            else 
                return ml::ret::fail("Llm call failed :\n " + res.dump(4));
        }

        std::string llmresp = "";
        if (res.contains("data") && res["data"].contains("content"))
        {
            for (auto& m : res["data"]["content"])
            {
                if (m.contains("text"))
                    llmresp += m["text"];
            }
        }

        _writeWhatItReceived(llmresp);

        _outData = "";
        if (_afterLlmCall.empty())
            _outData = llmresp;
        else 
        {
            for(const auto& f : _afterLlmCall)
                _outData += f(_inData, llmresp) + "\n";
        }
        if (_outData.size() > 0 && _outData[_outData.size() - 1] == '\n')
            _outData.pop_back();

        return ml::ret::success();
    }

    json Agent::serialize() const
    {
        json config;
        config["id"] = _id;
        config["name"] = _name;
        config["context"] = _context;
        config["inData"] = _inData;
        config["outData"] = _outData;
        config["config"] = _config.serialize();
        config["description"] = _description;
        return config;
    }

    void Agent::deserialize(const json& config)
    {
        if (config.contains("id")) _id = config["id"].get<std::string>();
        if (config.contains("name")) _name = config["name"].get<std::string>();
        if (config.contains("context")) _context = config["context"].get<std::string>();
        if (config.contains("inData")) _inData = config["inData"].get<std::string>();
        if (config.contains("outData")) _outData = config["outData"].get<std::string>();
        if (config.contains("config")) _config.deserialize(config["config"]);
        if (config.contains("description")) _description = config["description"].get<std::string>();
    }
}
