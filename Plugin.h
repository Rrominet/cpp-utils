#pragma once
#include <string>
#include "./mlprocess.h"
#include <functional>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ml
{
    struct PluginOut
    {
        ProcessOut processOut;
        std::string path;
    };

    class Plugin
    {
        public : 
            enum State { NOT_STARTED, RUNNING, DONE, ERROR };
            Plugin(const std::string& path="") : _path(path) {};
            Plugin(const Plugin& p) : _path(p._path), _state(p._state.load()), _error(p._error) {};
            Plugin& operator=(const Plugin& p) { _path = p._path; _state = p._state.load(); _error = p._error; return *this; };

            PluginOut exec(const std::string& executorPath, const json& data);

            //f is called on another thread !
            void exec_async(const std::string& executorPath, const json& data, const std::function<void(PluginOut)> &f);

            std::string name() const;

        private : 
            std::string _path; //bp cgs
            std::atomic<State> _state = NOT_STARTED; //bp cg
            std::string _error; //bp cg

            ProcessOut _execProcess(const std::string& executorPath, const std::string& stdin);

        public : 
#include "./Plugin_gen.h"
    };

    bool operator==(const ml::Plugin& a, const ml::Plugin& b);
    bool operator!=(const ml::Plugin& a, const ml::Plugin& b);
}
