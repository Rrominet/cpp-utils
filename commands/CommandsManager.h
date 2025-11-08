#pragma once
#include "./Command.h"
#include "./MacroCommand.h"
#include "./vec.h"
#include "../HistoryStack.h"
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "./JsonCommand.h"
#include "./ProcessCommand.h"

namespace ml
{
    class CommandsManager
    {
        public:
            // if id is empty, it will be the name
            template<typename T=Command, typename A=std::any>
                std::shared_ptr<T> createCommand(const std::string name, std::string id="", const std::function<void(const std::any&)>& exec = 0, const A& args={})
                {
                    std::shared_ptr<T> cmd = std::make_shared<T>();
                    cmd->setName(name);
                    if (exec)
                    {
                        lg("exec is def, so set it");
                        cmd->setExec(exec);
                    }
                    else 
                    {
                        lg("exec is not def, so do nothing");
                    }
                    cmd->setArgs(args);

                    if (id == "")
                        id = name;

                    cmd->setId(id);
                    _commands[id] = cmd;
                    return std::dynamic_pointer_cast<T>(cmd);
                }

            template<typename T=MacroCommand, typename A=std::any>
                std::shared_ptr<T> createMacro(const std::string name, std::string id="")
                {
                    std::shared_ptr<T> cmd = std::make_shared<T>(this);
                    cmd->setName(name);

                    if (id == "")
                        id = name;

                    cmd->setId(id);
                    _commands[id] = cmd;
                    return std::dynamic_pointer_cast<T>(cmd);
                }

            
            // the template are here because these type are treated for mlgui only.
            // if not setted, they won't be treated.
            template<typename Gui=void, typename GuiBackend=void>
                void deserialize(const json& j)
                {
                    if (j.contains("commands"))
                    {
                        for (const auto& cmd : j["commands"])
                        {
                            if (!cmd.contains("id"))
                                continue;

                            if (!cmd.contains("type"))
                                continue;

                            std::shared_ptr<Command> _cmd;
                            bool founded = false;
                            try
                            {
                                _cmd = this->command(cmd["id"]);
                                founded = true;
                            }
                            catch(const std::exception& e)
                            {
                                if (cmd["type"] == "command")
                                    _cmd = std::make_shared<Command>();
                                else if (cmd["type"] == "macroCommand")
                                    _cmd = std::make_shared<MacroCommand>(this);
                                else if (cmd["type"] == "jsonCommand")
                                    _cmd = std::make_shared<JsonCommand>();
                                else if (cmd["type"] == "processCommand")
                                    _cmd = std::make_shared<ProcessCommand>();

                                if constexpr (!std::is_same_v<Gui, void>)
                                {
                                    if (cmd["type"] == "guiCommand")
                                        _cmd = std::make_shared<Gui>();
                                }

                                if constexpr (!std::is_same_v<GuiBackend, void>)
                                {
                                    if (cmd["type"] == "guiBackendCommand")
                                        _cmd = std::make_shared<GuiBackend>();
                                }
                            }

                            _cmd->deserialize(cmd);
                            if (!founded)
                                _commands[cmd["id"]] = _cmd;
                        }
                    }
                }

            std::shared_ptr<Command> exec(const std::string& id);
            std::shared_ptr<Command> reverse(const std::string& id);

            std::shared_ptr<Command> command(const std::string& id);
            bool exists(const std::string& id) { return _commands.find(id) != _commands.end(); }
            bool has(const std::string& id) { return this->exists(id); }
            void removeCommand(const std::string& id);
            std::vector<std::string> lsIds();
            void logIds();

            // TODO need to test it.
            void undo();
            void redo();

            json serialize() const;

            std::unordered_map<std::string, std::shared_ptr<Command>>& commands() { return _commands; }
            const std::unordered_map<std::string, std::shared_ptr<Command>>& commands() const { return _commands; }

        private : 
            std::unordered_map<std::string, std::shared_ptr<Command>> _commands;
            HistoryStack<std::string> _history; //bp cg
                                                                       
        public : 
#include "./CommandsManager_gen.h"
    };
}
