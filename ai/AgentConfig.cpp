#include "./AgentConfig.h"
#include "../files.2/files.h"

namespace ml
{
    AgentConfig fromConfig(const json& config)
    {
        AgentConfig a;
        a.deserialize(config);
        return a;
    }

    AgentConfig fromFile(const std::string& configpath)
    {
        AgentConfig a;
        a.deserialize(files::read(configpath));
        return a;
    }

    json AgentConfig::serialize() const
    {
        json config;
        config["api_keys"] = _api_keys;
        config["max_tokens"] = _max_tokens;
        config["model"] = _model;
        config["logFile"] = _logFile;
        return config;
    }

    void AgentConfig::deserialize(const json& config)
    {
        if (config.contains("api_keys")) _api_keys = config["api_keys"].get<std::string>();
        if (config.contains("max_tokens")) _max_tokens = config["max_tokens"].get<int>();
        if (config.contains("model")) _model = config["model"].get<std::string>();
        if (config.contains("logFile")) _logFile = config["logFile"].get<std::string>();
    }
}
