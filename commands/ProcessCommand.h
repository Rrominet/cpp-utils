#pragma once
#include "./Command.h"

namespace ml
{
    class ProcessCommand : virtual public Command
    {
        public:
            ProcessCommand() = default;
            ~ProcessCommand() = default;

            virtual void exec() override;

            virtual json serialize() const override;
            virtual void deserialize(const json& j) override;

        protected : 
            bool _detached = true; //bp cgs
            int _exitCode = 0; //bp cg
            std::string _stdout; //bp cg
            std::string _stderr; //bp cg
            std::string _processPath; //bp cgs
            ml::Vec<std::string> _processArgs; //bp cgs

        public :
#include "./ProcessCommand_gen.h"
    };
}
