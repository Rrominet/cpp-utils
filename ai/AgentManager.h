#pragma once
#include "../vec.h"
#include "../mlprocess.h"
#include "./Agent.h"
#include <unordered_map>

namespace ml
{
    class AgentManager
    {
        public:
            AgentManager(const std::string& llmProcessPath="") : _llmProcessPath(llmProcessPath) {}
            ~AgentManager() {_llm.terminate();}

            const std::unordered_map<std::string, Agent>& agents() const { return _agents; }
            
            template<typename T>
                void addAgent(const T& agent) { _agents[agent.id()] = agent;}

            void removeAgent(const std::string& id) { _agents.erase(id); }
            void clear() { _agents.clear(); }

            Ret<std::string> execAgent(const std::string& id, const std::string& message="");
        
            //each agent get the _outData of the previous agent in _inData
            Ret<std::string> chainAgents(const ml::Vec<std::string> ids, const std::string& message="");

            //this can be called before or after adding agents, this is setted just before any execution.
            //the exceptIds wont be touched
            void applyConfigToAllAgents(const AgentConfig& config, const ml::Vec<std::string>& exceptIds={});
            void applyConfigToAllAgents(const json& config, const ml::Vec<std::string>& exceptIds={});
            void applyConfigToAllAgents(const std::string& configpath, const ml::Vec<std::string>& exceptIds={});

        private : 
            json _globalConfig = json::object(); //bp cg
            ml::Vec<std::string> _globalConfigExceptIds = ml::Vec<std::string>(); //bp cgs

            std::unordered_map<std::string, Agent> _agents;
            Process _llm; //bp cg
            std::string _llmProcessPath; //bp cgs

            Ret<> _startLlmProcess();

        public : 
#include "./AgentManager_gen.h"
    };
}
