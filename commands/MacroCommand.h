#pragma once
#include "./Command.h"
#include "./vec.h"


// this macro command class would only works if the command added has been created with the CommandManager passed as argument in the constructor
// the MacroCommand itseld should be created with the CommandManager not from this constructor ! (use the the createMacro method, not the createCommand one)
// generally all command should be created with the CommandManager not their respective constructors unless really specefic scenarios.
namespace ml
{
    class CommandsManager;
    class MacroCommand : public Command
    {
        public:
            MacroCommand(CommandsManager* cmdsMgr);
            virtual ~MacroCommand(){}
            void add(const std::string& id){_cmdsIds.add(id);}
            void remove(const std::string& id){_cmdsIds.remove(id);}
            void remove(unsigned int index){_cmdsIds.removeByIndex(index);}
            void clear(){_cmdsIds.clear();}

            virtual json serialize() const;
            virtual void deserialize(const json& j);

        protected : 
            ml::Vec<std::string> _cmdsIds; //bp cgs
            CommandsManager* _cmdsMgr;

        public : 
#include "./MacroCommand_gen.h"
    };
}
