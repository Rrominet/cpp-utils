#include "./Agent.h"
#include "../mlTime.h"
#include "../files.2/files.h"
#include "../ipc.h"
#include "./utils.h"

namespace ml 
{

    json LLMessage::asjson() const
    {
        json r;	
        r["role"] = this->role;
        r["content"] = this->content;
        return r;
    }

    void LLMessage::setFromJson(const json& j)
    {
        if (j.contains("role"))
            this->role = j["role"];
        if (j.contains("content"))
            this->content = j["content"];	
    }

    LLMessage LLMessage::fromJson(const json& j)
    {
        LLMessage r;	
        r.setFromJson(j);
        return r;
    }
    const std::string Agent::name() const
    {
        if(!_name.empty())
            return _name;
        return _id;
    }

    void Agent::_writeLog(const std::string& msg)
    {
        std::string data;
        data = ml::time::asString(ml::time::now()) + ":\n" + msg;
        try
        {
            lg(data);
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

    Ret<> Agent::exec(Process* llm, bool onlyLog)
    {
        lg("Agent : " + _id + " is exec(" << llm << ")");
        if (!llm)
            return ml::ret::fail("The llm process is null.");
        if (_inData.empty() && _llmMessages.empty())
            return ml::ret::fail("The in data is empty (no inData or llmMessages). Nothing to send to the llm.");

        lg("Executing the _beforeLlmCall functions : " << _beforeLlmCall.size());
        std::string _beforeRes;
        for (unsigned int i = 0; i < _beforeLlmCall.size(); i++)
        {
            lg("Executing the _beforeLlmCall function : " << i);
            auto res = _beforeLlmCall[i](this, _inData);
            if (!res.empty())
                _beforeRes += res + "\n";
            lg("_beforeLlmCall[" << i << "] res : " << res);
        }

        lg("Additional data from the functions called before the llm : " << _beforeRes);
        if (!_beforeRes.empty())
        {
            _beforeRes.pop_back();
            _inData += "\nMore information to help you complete your task : \n" + _beforeRes;
        }

        if (!_error.empty())
            return ml::ret::fail(_error);

        _writeWhatItSent();
        json pdata;
        pdata["context"] = _context;
        pdata["messages"] = json::array();
        pdata["model"] = _config.model();
        pdata["model-class"] = _config.modelVendor();
        pdata["api-key"] = _config.api_keys();
        pdata["max-tokens"] = _config.max_tokens();
        pdata["tools"] = json::array();

        //setting the messages
        for (const auto& msg : _llmMessages)
            pdata["messages"].push_back(msg.asjson());

        if (!_inData.empty())
        {
            if (_llmMessages.empty() ||  _llmMessages.back().role == "assistant" )
                pdata["messages"].push_back({{"role", "user"}, {"content", _inData}});
            else 
                pdata["messages"].back()["content"] += "\n\n" + _inData;
        }

        //
        //
        if (pdata["messages"].empty())
            return ml::ret::fail("No messages to send to the llm in the compiled data.");

        if (_config.tools().contains("web_search"))
            pdata["tools"].push_back({{"type", "web_search"}});

        if (!_config.reasoning().empty())
        {
            json reasoning;
            reasoning["effort"] = _config.reasoning();
            reasoning["mode"] = _config.reasoningMode();
            pdata["reasoning"] = reasoning;
        }

        lg("Data sent to the llm wrapper : ");
        lg(pdata.dump(4));

        if (onlyLog)
        {
            _rawllm = "This is some test data, the LLM have not really been called.";
            _outData = _rawllm;
            return ml::ret::ok();
        }

        auto res = ipc::call_sync(llm, "send", pdata);
        lg("LLM wrapper called.");
        lg("Response received : " << res.dump(4));
        if(!res.contains("success") || !res["success"])
        {
            if (res.contains("message"))
                return ml::ret::fail("Llm call failed :\n " + res["message"].get<std::string>());
            else if (res.contains("error"))
                return ml::ret::fail("Llm call failed :\n " + res["error"].get<std::string>());
            else 
                return ml::ret::fail("Llm call failed :\n " + res.dump(4));
        }

        _rawllm = "";
        if (res.contains("data") && res["data"].contains("content"))
        {
            for (auto& m : res["data"]["content"])
            {
                if (m.contains("text"))
                    _rawllm += m["text"];
            }
        }

        //TODO add all the infos concerning token cost and the rest of the data given by the llm vendor !

        _writeWhatItReceived(_rawllm);

        lg("Reinitializing the _outData.");
        _outData = "";
        if (_afterLlmCall.empty())
        {
            lg("_afterLlmCall is empty, so _outData = _rawllm");
            _outData = _rawllm;
        }
        else 
        {
            lg("executing the _afterLlmCall " << _afterLlmCall.size() << " functions.");
            for(unsigned int i = 0; i < _afterLlmCall.size(); i++)
            {
                lg("Executing the _afterLlmCall function : " << i);
                const auto& f = _afterLlmCall[i];
                _outData += f(this, _inData, _rawllm) + "\n";
                lg("_afterLlmCall[" << i << "] res : " << _outData);
            }
        }
        if (_outData.size() > 0 && _outData[_outData.size() - 1] == '\n')
        {
            lg("Removing last char '\n' from _outData.");
            _outData.pop_back();
        }

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

    void Agent::addProjectContext(const std::string& prj)
    {
        this->setContext(this->context() + "\nHere is the full context for the project your're working on :\n" + prj) ;
    }

    Ret<> Agent::addProjectContextFile(std::string filepath)
    {
        if (!files::exists(filepath))
            filepath = _root + files::sep() + filepath;
        try
        {
            this->addProjectContext(files::read(filepath)); 
        }
        catch(const std::exception& e)
        {
            return ml::ret::fail("Can't read the file " + filepath + " for setting the project context for the Agent : " + std::string(e.what()));
        }
        return ml::ret::ok();
    }

    void Agent::addFiles(const ml::Vec<std::string>& files)
    {
        auto files_s = ml::ai::filesForLLM(files.vec, _root);
        this->setContext(this->context() + "\nHere are the content of the files you need for context :\n" + files_s);
    }

    void Agent::addWritingRules(const std::string& rules)
    {
        this->setContext(this->context() + "\nHere are your writing rules you NEED to follow :\n" + rules); 
    }
}
