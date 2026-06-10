#pragma once
#include "./Agent.h"

namespace ml
{
    class BashAgent : public Agent
    {
        public : 
            BashAgent(const std::string& name="");
            std::string execBash(const std::string& inData, const std::string& outData);
            std::string bashPath();

            void checkBashPath();
            
        private : 
            std::string _generateCtx();
    };
}
