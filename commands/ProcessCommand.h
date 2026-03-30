#pragma once
#include "./Command.h"

namespace ml
{
    class ProcessCommand : public Command
    {
        public:
            ProcessCommand() = default;
            ~ProcessCommand() = default;

            virtual void exec() override;

            virtual json serialize() const override;
            virtual void deserialize(const json& j) override;

        protected : 
            std::string _processPath; //bp cgs
            ml::Vec<std::string> _processArgs; //bp cgs

        public :
#include "./ProcessCommand_gen.h"
    };
}
