#include <functional>
#include <unordered_map>
#include <boost/process.hpp>

#include "./scripts.h"
#include "vec.h"
#include "thread.h"
#include "json.h"
#include "files.2/files.h"

#include <mutex>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace scripts
{
    std::unordered_map<std::string, std::string> _enginesFlags{
        {"python", "-c"},
            {"py", "-c"},
            {"node", "-e"},
            {"php", "-r"},
    };

    ml::Vec<Script> _scripts;
    std::mutex _scriptsMutex;

    std::unordered_map<Event, ml::Vec<unsigned int>> _eventsMap;
    namespace parser
    {
        std::unordered_map<std::string, std::function<std::string()>> _api; 
        std::unordered_map<std::string, std::function<void(const json&)>> _cmds; 

        std::mutex _apiMutex;
        std::mutex _cmdsMutex;
    }

    // Copy constructor
    Script::Script(const Script& other)
        : _content(other._content),
        filepath(other.filepath),
        async(other.async),
        reload(other.reload),
        engine(other.engine),
        linkedEvents(other.linkedEvents)
    {}

    // Copy assignment operator
    Script& Script::operator=(const Script& other)
    {
        if (this == &other)
            return *this; // Handle self-assignment

        *this = Script(other);
        return *this;
    }

    std::string Script::content()
    {
        if (_content.empty() || this->reload) 
        {
            if (!filepath.empty())
            {
                _content = files::read(filepath);
                if (files::ext(filepath) == "php")
                    _content = str::replace(_content, "<?php", "");
            }
            else 
                throw std::runtime_error("No filepath set for script.");
        }
        return _content;
    }

    std::string Script::findEngineFlag() const
    {
        if (this->engine.empty())
            throw std::runtime_error("findEngineFlag : No engine set for this script : " + this->filepath);

        for (auto &f : _enginesFlags)
        {
            if (str::contains(this->engine, f.first))
                return f.second;
        }

        throw std::runtime_error("findEngineFlag : No engine flag found for this script : " + this->filepath);
    }

    void Script::run(const std::function<void(const std::string&)>& func, const std::function<void(const std::string&)>& onError)
    {
        auto script = *this;
        auto f = [script, func, onError] () mutable{
            try
            {
                if (script.engine.empty()) 
                    throw std::runtime_error("No engine set for executing script : " + script.filepath);

                auto toRun = parser::constentParsed(script);
                std::vector<std::string> cmd {script.engine, script.findEngineFlag(), toRun};

                boost::process::ipstream is;
                boost::process::ipstream is_err;
                auto c = boost::process::child(cmd, boost::process::std_out > is, boost::process::std_err > is_err);
                std::string std_out;
                std::string std_err;
                std::string line;
                std::string line_err;

                while(std::getline(is, line))
                    std_out += line + "\n";
                std_out.pop_back();

                while(std::getline(is_err, line))
                {
                    std::cerr << line << std::endl;
                    std_err += line + "\n";
                }
                std_err.pop_back();

                c.wait();
                if (c.exit_code() == 0)
                {
                    parser::execCmdsOnStdout(std_out);
                    if (func)
                        func(std_out);
                }
                else 
                {
                    if (onError)
                        onError(std_err);
                    else
                    {
                        if (!script.async)
                            throw std::runtime_error(std_err);
                    }
                }
            }
            catch(const std::exception& e)
            {
                if (onError)
                    onError(e.what());
                else 
                {
                    if (!script.async)
                        throw;
                    else
                        std::cerr << e.what() << std::endl;
                }
            }
        };

        if (this->async)
            std::thread(f).detach();
        else
            f();
    }

    std::string Script::name() const
    {
        return str::split(filepath, files::sep()).back();
    }

    void Script::save(const std::string& scriptsdir) const
    {
        json j; 
        j["filepath"] = str::replace(this->filepath, scriptsdir, "$root");
        j["engine"] = this->engine;
        j["async"] = this->async;
        j["linkedEvents"] = ml::json::serializeSimpleList(this->linkedEvents);
        files::write(scriptsdir + files::sep() + this->name() + ".conf", j.dump());
    }

    void Script::install(const std::string& scriptsdir)
    {
        files::copy(this->filepath, scriptsdir + this->name());
        this->filepath = "$root" + files::sep() + this->name();
        this->save(scriptsdir);
    }

    void Script::read(const std::string& scriptsdir, unsigned int idx)
    {
        if (this->filepath.empty())
            throw std::runtime_error("Script::read : No filepath set for script : " + this->filepath);
        auto cffile = scriptsdir + files::sep() + this->name() + ".conf";
        if (!files::exists(cffile))
            throw std::runtime_error("Script::read : No config file found for script : " + this->filepath);

        json j = json::parse(files::read(cffile));	
        this->filepath = j["filepath"];
        this->filepath = str::replace(this->filepath, "$root", scriptsdir);
        this->engine = j["engine"];
        this->async = j["async"];
        if (j.contains("linkedEvents"))
            this->linkedEvents = ml::json::asVector<Event>(j["linkedEvents"]);
    }

    namespace parser
    {

        void addFunction(const std::string &key, const std::function<std::string()>& func)
        {
            LK(_apiMutex);
            _api[key] = func;
        }

        void removeFunction(const std::string &key)
        {
            LK(_apiMutex);
            _api.erase(key);
        }

        void clearFunctions()
        {
            LK(_apiMutex);
            _api.clear();
        }


        std::string quoted(const std::string& str)
        {
            return "\"" + str + "\"";
        }

        void addCmd(const std::string &key, const std::function<void(const json&)>& func)
        {
            LK(_cmdsMutex);
            _cmds[key] = func;
        }

        void removeCmd(const std::string &key)
        {
            LK(_cmdsMutex);
            _cmds.erase(key);
        }

        void clearCmds()
        {
            LK(_cmdsMutex);
            _cmds.clear();
        }

        void execCmdsOnStdout(const std::string& std_out)
        {
            LK(_cmdsMutex);
            if (_cmds.empty())
                return;
            try
            {
                json j = json::parse(std_out);
                if (!j.contains("cmd"))
                    return;

                _cmds[j["cmd"].get<std::string>()](j);
            }
            catch(const std::exception& e)
            {
                return;
            }
        }

        std::string constentParsed(Script& script)
        {
            auto c = script.content();
            LK(_apiMutex);
            for (auto &f : _api)
                c = str::replace(c, f.first, f.second());
            return c;
        }
    }

    std::string run(const std::string& filepath, const std::string& engine)
    {
        Script script;
        script.filepath = filepath;
        script.engine = engine;
        script.async = false;
        std::string _res = "";

        script.run([&_res](const std::string& res){
                _res = res;
                });

        return _res;
    }

    void run_async(const std::string& filepath, const std::string& engine, const std::function<void(const std::string&)>& func, const std::function<void(const std::string&)>& onError)
    {
        Script script;
        script.filepath = filepath;
        script.engine = engine;
        script.async = true;
        script.run(func, onError);
    }

    // the return value is the index to access the script later via scripts::_()[i]
    unsigned int create(const std::string& filepath, const std::string& engine, bool async)
    {
        Script script;
        script.filepath = filepath;
        script.engine = engine;
        script.async = async;
        _scripts.push_back(script);
        return _scripts.size() - 1;
    }

    Script& get(unsigned int idx)
    {
        return _scripts[idx];
    }

    unsigned int scriptFromName(const std::string& name)
    {
        for (unsigned int i = 0; i < _scripts.size(); i++)
            if (_scripts[i].name() == name)
                return i;
        throw std::runtime_error("scriptFromName : No script found with name : " + name);
    }

    unsigned int scriptFromFilepath(const std::string& filepath)
    {
        for (unsigned int i = 0; i < _scripts.size(); i++)
            if (_scripts[i].filepath == filepath)
                return i;
        throw std::runtime_error("scriptFromFilepath : No script found with filepath : " + filepath);
    }

    void remove(unsigned int idx)
    {
        for (auto& ev : _eventsMap)
            ev.second.remove(idx);

        _scripts.remove(idx);
    }

    void linkToEvent(unsigned int scriptIdx, Event event)
    {
        _eventsMap[event].push_back(scriptIdx);
        auto script = scripts::get(scriptIdx);
        if (!script.linkedEvents.contains(event))
            scripts::get(scriptIdx).linkedEvents.push_back(event);
    }

    void runEvent(Event event, const std::function<void(const std::string&)>& func, const std::function<void(const std::string&)>& onError)
    {
        for (auto idx : _eventsMap[event])
            scripts::get(idx).run(func, onError);
    }

    void init(const std::string& dirpath)
    {
        ml::Vec<Script> scripts;
        for (auto& f : files::ls(dirpath))
        {
            if (files::ext(f) != "conf")
            {
                try
                {
                    Script s;
                    s.filepath = f;
                    {
                        s.read(dirpath, scripts.size());
                        scripts.push_back(s);
                    }
                }
                catch(const std::exception& e)
                {
                    lg("Error while reading script " + f + " : " + e.what());
                    continue;
                }
            }
        }

        LK(_scriptsMutex);
        _scripts = std::move(scripts);
        for (unsigned int i = 0; i < _scripts.size(); i++)
        {
            auto& s = _scripts[i];
            for (auto& ev : _scripts[i].linkedEvents)
                linkToEvent(i, ev);
        }
    }

    void init_async(const std::string& dirpath, const std::function<void()>& onDoned, const std::function<void(const std::string&)>& onError)
    {
        auto f = [dirpath, onDoned, onError](){
            try
            {
                init(dirpath);
                if (onDoned)
                    onDoned();
            }
            catch(const std::exception& e)
            {
                if (onError)
                    onError(e.what());
            }
        };
        std::thread(f).detach();
    }

    void save(const std::string& dirpath, bool install)
    {
        LK(_scriptsMutex);
        for (auto& s : _scripts)
        {
            if (install)
                s.install(dirpath);
            else
                s.save(dirpath);
        }
    }

    void save_async(const std::string& dirpath, bool install, const std::function<void()>& onDoned, const std::function<void(const std::string&)>& onError )
    {
        auto f = [dirpath, install, onDoned, onError](){
            try
            {
                save(dirpath, install);
                if (onDoned)
                    onDoned();
            }
            catch(const std::exception& e)
            {
                if (onError)
                    onError(e.what());
            }
        };
        std::thread(f).detach();
    }
}
