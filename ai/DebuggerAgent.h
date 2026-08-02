#pragma once
#include "./Agent.h"

namespace ml
{
    class DebuggerAgent : public Agent
    {
        public:
            struct Issue
            {
                //critical, high, medium, low
                std::string type = "critical";
                std::string content;
            };

            DebuggerAgent(const std::string& name="");
            std::string processOutput(const std::string& inData, const std::string& outData);

            static ml::Ret<Issue> fromString(const std::string& s);
            const std::vector<Issue>& issues() const { return _issues; }

        private:
            std::string _generateCtx();
            std::vector<Issue> _issues;
    };
}
