#include "./MacroCommand.h"
#include "./CommandsManager.h"

namespace ml
{
    MacroCommand::MacroCommand(CommandsManager* cmdsMgr) : Command()
    {
        _cmdsMgr = cmdsMgr;

        //TODO need to be tested.
        auto toExec = [this](const std::any& args)
        {
            for (const auto& id : _cmdsIds)
                _cmdsMgr->command(id)->exec();
        };

        auto toReverse = [this](const std::any& args)
        {
            for (unsigned int i = _cmdsIds.size() - 1; i >= 0; i--)
                _cmdsMgr->command(_cmdsIds[i])->reverse();
        };

        this->setExec(toExec);
        this->setReverse(toReverse);
    }

    json MacroCommand::serialize() const
    {
        json _r = Command::serialize();
        _r["type"] = "macroCommand";
        _r["cmdsIds"] = json::array();
        for (const auto& id : _cmdsIds)
            _r["cmdsIds"].push_back(id);
        return _r;
    }

    void MacroCommand::deserialize(const json& j)
    {
        Command::deserialize(j);
        if (j.contains("cmdsIds"))
        {
            _cmdsIds.clear();
            for (const auto& id : j["cmdsIds"])
                _cmdsIds.push_back(id);
        }
    }
}
