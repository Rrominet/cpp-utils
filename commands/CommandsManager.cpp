#include "./CommandsManager.h"
#include "str.h"
#include "debug.h"

namespace ml
{
    std::shared_ptr<Command> CommandsManager::exec(const std::string& id)
    {
        _commands.at(id)->exec(); 
        _history.store(id);
        return _commands.at(id);
    }

    std::shared_ptr<Command> CommandsManager::reverse(const std::string& id)
    {
        _commands.at(id)->reverse();
        return _commands.at(id);
    }

    void CommandsManager::undo()
    {
        try
        {
            _commands.at(_history.current())->reverse();
            _history.undo();
        }
        catch(const std::exception& e)
        {
            lg(e.what());
        }
    }

    void CommandsManager::redo()
    {
        try
        {
            _commands.at(_history.current())->exec();
            _history.redo();
        }
        catch(const std::exception& e)
        {
            lg(e.what());
        }
    }

    std::shared_ptr<Command> CommandsManager::command(const std::string& id)
    {
        try
        {
            return _commands.at(id);
        }
        catch(const std::exception& e)
        {
            std::string _error = "Command " + id + " not found.";
            _error += _S"\n" + e.what();
            lg(_error);
            throw std::runtime_error(_error);
        }
    }

    std::vector<std::string> CommandsManager::lsIds()
    {
        std::vector<std::string> ids;
        for (auto& cmd : _commands)
            ids.push_back(cmd.first);
        return ids;
    }

    void CommandsManager::logIds()
    {
        db::log(this->lsIds()); 
    }

    json CommandsManager::serialize() const
    {
        json _r = json::object();
        _r["commands"] = json::object();
        for (auto& cmd : _commands)
            _r["commands"][cmd.first] = cmd.second->serialize();
        return _r;
    }

    void CommandsManager::removeCommand(const std::string& id)
    {
        try
        {
            _commands.erase(id);
        }
        catch(const std::exception& e)
        {
            lg(e.what());
        }
    }
}
