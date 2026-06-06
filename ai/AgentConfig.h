#pragma once
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


namespace ml
{
    class AgentConfig
    {
        public:
            AgentConfig(const std::string& api_keys="", int max_tokens=0, const std::string& model="", const std::string& logFile="") : _api_keys(api_keys), _max_tokens(max_tokens), _model(model), _logFile(logFile) {};
            static AgentConfig fromConfig(const json& config);
            static AgentConfig fromFile(const std::string& configpath);

            json serialize() const;
            void deserialize(const json& config);

        private : 
            std::string _api_keys; //bp cgs
            int _max_tokens = 0; //bp cgs
            std::string _model; //bp cgs
            std::string _logFile; //bp cgs

        public : 
#include "./AgentConfig_gen.h"
    };
}
