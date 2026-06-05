#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <atomic>
#include "./Plugin.h"
#include "./Ret.h"

namespace ml
{
    class PluginManager
    {
        public : 
            enum State {NOT_LOADED, LOADING, LOADED};

            PluginManager(const std::string& path="", const std::string& executorPath="") : _path(path), _executorPath(executorPath) {}

            Ret<> load();

            //f is executed on another thread.
            void load_async(const std::function<void(ml::Ret<>)> &f=0);

            // onDone is executed on another thread for EACH plugin executed.
            Ret<> emit(const std::string& eventName, const json& data, const std::function<void(PluginOut)> &onDone=0);

            Ret<> enable(const std::string& eventName, const std::string& pluginFileName);
            Ret<> disable(const std::string& eventName, const std::string& pluginFileName);
            bool enabled(const std::string& eventName, const std::string& pluginFileName);

            //TODO : should be possible to install directories too, not only files
            //and so having a different verson of this function that takes a ml::Vec<std::string> for the filepaths
            Ret<> install(const std::string& eventName, const std::string& filepath);
            Ret<> uninstall(const std::string& eventName, const std::string& pluginFileName);

        private : 
            std::atomic<State> _state = NOT_LOADED; //bp cg
            std::string _path; //bp cgs
            std::string _executorPath; //bp cgs
            std::unordered_map<std::string, ml::Vec<Plugin>> _plugins; //bp cg

            //format : 
            //{
            //    "event-name" :{
            //         "plugin-name" : {"enabled" : true/false}, ...
            //    }  
            //}
            json _metadata = json::object();
            std::string _metatadataPath();
            void _saveMetaData();
            void _readMetaData();

            Ret<> _setEnabled(const std::string& eventName, const std::string& pluginFileName, bool value);

            //throw an exception if not found
            Plugin& _get(const std::string& eventName, const std::string& pluginFileName);

        public : 
#include "./PluginManager_gen.h"
    };
}
