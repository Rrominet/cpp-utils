#pragma once
#include "./Agent.h"

namespace ml
{
    class DebuggerAgent : public Agent
    {
        public:
            DebuggerAgent(const std::string& name="");
            std::string processOutput(const std::string& inData, const std::string& outData);

            const std::vector<std::string>& issues() const { return _issues; }

        private:
            std::string _generateCtx();
            std::vector<std::string> _issues;
    };
}
