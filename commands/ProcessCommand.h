#pragma once
#include "./Command.h"
#include "../mlprocess.h"

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

        public :
#include "./ProcessCommand_gen.h"
    };
}
