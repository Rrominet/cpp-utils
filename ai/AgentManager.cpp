#include "./AgentManager.h"
#include "../files.2/files.h"
#include "../str.h"

namespace ml
{
    Ret<std::string> AgentManager::execAgent(const std::string& id, const std::string& message)
    {
        auto ret = _startLlmProcess(); 
        if (!ret.success)
            return ml::ret::fail<std::string>(ret.message);

        if (_agents.find(id) == _agents.end())
            return ml::ret::fail<std::string>("The agent " + id + " is not found.");

        auto cp = _agents[id];
        if (!_globalConfigExceptIds.contains(id))
            cp.config().deserialize(_globalConfig);

        cp.setInData(message);
        auto r = cp.exec(&_llm);
        if (!r.success)
            return ml::ret::fail<std::string>(r.message);
        return ml::ret::ok<std::string>(cp.outData());
    }

    Ret<std::string> AgentManager::chainAgents(const ml::Vec<std::string> ids, const std::string& message)
    {
        if (ids.empty())
            return ml::ret::fail<std::string>("The ids list is empty.");

        auto ret = _startLlmProcess(); 
        if (!ret.success)
            return ml::ret::fail<std::string>(ret.message);

        ml::Vec<std::string> _notFounded;
        for (const auto& id : ids)
        {
            if (_agents.find(id) == _agents.end())
                _notFounded.push_back(id);
        }

        if (_notFounded.size() > 0)
            return ml::ret::fail<std::string>("The agents " + str::join(_notFounded, ", ") + " are not found.");

        std::string errors;
        ml::Vec<Agent> _agentsCopy;
        for (const auto& id : ids)
            _agentsCopy.push_back(_agents[id]);

        for (auto& agent : _agentsCopy)
        {
            if (!_globalConfigExceptIds.contains(agent.id()))
                agent.config().deserialize(_globalConfig);
        }

        for (unsigned int i = 0; i < ids.size(); i++)
        {
            if (i == 0)
                _agentsCopy[i].setInData(message);
            if (i > 0 && i < ids.size() - 1)
                _agentsCopy[i].setInData(_agentsCopy[i - 1].outData());

            auto r = _agentsCopy[i].exec(&_llm);
            if (!r.success)
                errors += "Error in the Agent " + ids[i] + " : " + r.message + "\n";
        }

        if (errors.empty())
            return ml::ret::ok<std::string>(_agentsCopy[ids.size() - 1].outData());
        else 
            return ml::ret::fail<std::string>(errors);
    }

    Ret<> AgentManager::_startLlmProcess()
    {
        if (_llm.running())
            return ml::ret::ok();

        if (!files::exists(_llmProcessPath))
            return ml::ret::fail("The llm process " + _llmProcessPath + " is not found.");

        _llm.setCmd_s(_llmProcessPath);
        _llm.start();

        return ml::ret::ok();
    }

    void AgentManager::applyConfigToAllAgents(const AgentConfig& config,const ml::Vec<std::string>& exceptIds)
    {
        _globalConfig = config.serialize();
        _globalConfigExceptIds = exceptIds;
    }

    void AgentManager::applyConfigToAllAgents(const json& config,const ml::Vec<std::string>& exceptIds)
    {
        _globalConfig = config;
        _globalConfigExceptIds = exceptIds;
    }

    void AgentManager::applyConfigToAllAgents(const std::string& configpath,const ml::Vec<std::string>& exceptIds)
    {
        try
        {
            _globalConfig = json::parse(files::read(configpath));
        }
        catch(const std::exception& e)
        {
            lg("error in applying config, json parse or file not found : " << configpath << "\n" << e.what());
        }
        _globalConfigExceptIds = exceptIds;
    }
}
