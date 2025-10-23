#pragma once
#include <map>
#include <type_traits>
#include <nlohmann/json.hpp>
#include "str.h"
#include "vec.h"
using json = nlohmann::json;

// the options are the arguments passed when starting the program. 
// the config is options that the user can save to disk permanently

class CmdPgr
{
    public : 
        CmdPgr();
        CmdPgr(int argc, char* argv[]);
        CmdPgr(std::map<std::string, std::string> options);
        virtual ~CmdPgr(){}

        virtual void run();
        virtual void test();
        virtual const std::string version() const {return "*version*";}

        //the options are the args passed to the program execution
        std::string option(const std::string& key);
        bool optionExistsAndEmpty(const std::string& key);
        std::map<std::string, std::string>& options();
        bool optionExists(const std::string& key){return _options.find(key) != _options.end();}

        virtual std::string configPath();

        std::string config(const std::string& key, bool readFromFile=true);
        template<typename T>
        void setConfig(const std::string& key, const T& value, const bool& save=true)
        {
            if constexpr (std::is_same_v<T, std::string>)
                _configs[key] = value;
            else if constexpr (std::is_same_v<T, bool>)
            {
                if (value)
                    _configs[key] = "true";
                else 
                    _configs[key] = "false";
            }
            else if constexpr (std::is_same_v<T, json>)
                _configs[key] = value.dump();
            else 
                _configs[key] = std::to_string(value);
            if (save)
                this->saveConfig(key);
        }

        std::string execPath();
        void readConfig();
        void reloadConfig();
        // if key.empty(), it save all
        void saveConfig(const std::string& key="");

        template<typename T>
        std::vector<T> fromString(const std::string& s)
        {
            std::vector<T> _r;
            if (s.empty())
                return _r;
            auto strs = str::split(s, ":");
            for (const auto& _s : strs)
                _r.push_back(T(_s));
            return _r;
        }

        void addToRecentFiles(const std::string& path);
        void removeFromRecentFiles(const std::string& path);
        ml::Vec<std::string> recentFiles();
        void clearRecentFiles();
        void saveRecentFiles(const ml::Vec<std::string>& recents);
        void log(const std::string& msg) {lg("CmdPgr::log not implemented");}
        bool needToLog() const {return _needToLog;}
        void setNeedToLog(bool val){_needToLog = val;}

        std::string input(const std::string& msg="") const;

        // to implement when needed
        // void foreach(config/options, func);

    protected : 
        // if local is set to true
        // .config dir will be created next to the exec file
        bool _local = false;
        std::map<std::string, std::string> _options;
        std::map<std::string, std::string> _configs;
        std::string _name;

        bool _configReaded = false;
        bool _needToLog = true;
        std::string _logFile;

        int _argc = 0;
        char** _argv = nullptr;

    private : 
        // intern use only
        void _saveConfigKey(const std::string& key);

};

namespace cmd 
{
    ml::Vec<std::string> filesFromArg(const std::string& arg);
    std::string argfromFiles(const ml::Vec<std::string>& files);
}
