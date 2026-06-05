#include "./PluginManager.h"
#include "./files.2/files.h"
#include "Ret.h"

namespace ml
{
    Ret<> PluginManager::load()
    {
        _state = State::LOADING;
        if (!files::isDir(_path))
        {
            _state = State::NOT_LOADED;
            return ml::ret::fail("Path " + _path + " is not a directory");
        }
        _readMetaData();

        _plugins.clear();
        ml::Vec<std::string> files = files::ls(_path); 
        ml::Vec<std::string> dirs;
        dirs.reserve(files.size());
        for (const auto& f : files)
        {
            if (files::isDir(f))
                dirs.push_back(f);
        }

        for (const auto& d : dirs)
        {
            auto pluginsFiles = files::ls(d);
            ml::Vec<Plugin> plugins;
            plugins.reserve(pluginsFiles.size());
            for (const auto& p : pluginsFiles)
            {
                if (files::isFile(p))
                    plugins.push_back(Plugin(p));
            }
            _plugins[files::name(d)] = plugins;

            if (!_metadata.contains(files::name(d)))
                _metadata[files::name(d)] = json::object();

            for (auto& p : plugins)
            {
                if (!_metadata[files::name(d)].contains(files::name(p.path())))
                {
                    _metadata[files::name(d)][files::name(p.path())] = json::object();
                    _metadata[files::name(d)][files::name(p.path())]["enabled"] = false;
                }
            }
        }

        _saveMetaData();
        
        _state = State::LOADED;
        return ml::ret::success();
    }

    //f is executed on another thread.
    void PluginManager::load_async(const std::function<void(ml::Ret<>)> &f)
    {
        auto _f = [this, f]()
        {
            auto res = this->load();
            if (f)
                f(res);
        };
        std::thread(_f).detach();
    }

    // onDone is executed on another thread for EACH plugin executed.
    Ret<> PluginManager::emit(const std::string& eventName, const json& data, const std::function<void(PluginOut)> &onDone)
    {
        if (_state != State::LOADED)
            return ml::ret::fail("The plugin manager is not loaded.\nYou need to load it before launching any event.");
        if (_plugins.find(eventName) == _plugins.end())
            return ml::ret::fail("The event " + eventName + " was not found.");
        if (!files::exists(_executorPath))
            return ml::ret::fail("The executor file " + _executorPath + " was not found.");
        if (files::isDir(_executorPath))
            return ml::ret::fail("The executor file " + _executorPath + " is a directory.");
        
        for (auto& p : _plugins[eventName])
        {
            if (!this->enabled(eventName, files::name(p.path())))
                continue;
            p.exec_async(_executorPath, data, onDone);
        }

        return ml::ret::success();
    }

    Ret<> PluginManager::_setEnabled(const std::string& eventName,const std::string& pluginFileName,bool value)
    {
        if (_state != State::LOADED)
            return ml::ret::fail("The plugin manager is not loaded.\nYou need to load it before enabling/disabling any plugin.");
        if (_plugins.find(eventName) == _plugins.end())
            return ml::ret::fail("The event " + eventName + " was not found.");
        for (auto& p : _plugins[eventName])
        {
            if (files::name(p.path()) == pluginFileName)
            {
                if (!_metadata.contains(eventName))
                    _metadata[eventName] = json::object();
                if (!_metadata[eventName].contains(pluginFileName))
                    _metadata[eventName][pluginFileName] = json::object();

                _metadata[eventName][pluginFileName]["enabled"] = value;
                _saveMetaData();
                return ml::ret::success();
            }
        }
        return ml::ret::fail("The plugin " + pluginFileName + " was not found in " + eventName);
    }

    Ret<> PluginManager::enable(const std::string& eventName,const std::string& pluginFileName)
    {
        return _setEnabled(eventName, pluginFileName, true);
    }

    Ret<> PluginManager::disable(const std::string& eventName,const std::string& pluginFileName)
    {
        return _setEnabled(eventName, pluginFileName, false);
    }

    bool PluginManager::enabled(const std::string& eventName,const std::string& pluginFileName)
    {
        try 
        {
            _get(eventName, pluginFileName) ;
            return _metadata[eventName][pluginFileName]["enabled"];
        }
        catch(const std::exception& e)
        {
            return false;
        }
    }

    std::string PluginManager::_metatadataPath()
    {
        return _path + "/metadata.json"; 
    }

    void PluginManager::_saveMetaData()
    {
        files::write(_metatadataPath(), _metadata.dump()); 
    }

    void PluginManager::_readMetaData()
    {
        if (files::exists(_metatadataPath())) 
        {
            _metadata = json::parse(files::read(_metatadataPath()));
            if (!_metadata.is_object())
                _metadata = json::object();
        }
        else 
            _metadata = json::object();
    }

    Plugin& PluginManager::_get(const std::string& eventName, const std::string& pluginFileName)
    {
        if (_state != State::LOADED)
            throw std::runtime_error("The plugin manager is not loaded.\nYou need to load it before getting any plugin.");
        
        if (_plugins.find(eventName) == _plugins.end())
            throw std::runtime_error("The event " + eventName + " was not found.");
        
        auto& plugins = _plugins[eventName];
        for (auto& p : plugins)
        {
            if (files::name(p.path()) == pluginFileName)
                return p;
        }
        
        throw std::runtime_error("The plugin " + pluginFileName + " was not found.");
    }

    Ret<> PluginManager::install(const std::string& eventName,const std::string& filepath)
    {
        if (!files::exists(filepath))
            return ml::ret::fail("The file " + filepath + " does not exist.");
        if (files::isDir(filepath))
            return ml::ret::fail("The file " + filepath + " is a directory.");

        if (!files::exists(_path + files::sep() + eventName))
            files::mkdir(_path + files::sep() + eventName);
        
        try
        {
            files::copy(filepath, _path + files::sep() + eventName + files::sep() + files::name(filepath));
        }
        catch(const std::exception& e)
        {
            return ml::ret::fail("The file " + filepath + " could not be installed.\nMore infos : " + e.what());
        }

        return ml::ret::success();
    }

    Ret<> PluginManager::uninstall(const std::string& eventName,const std::string& pluginFileName)
    {
        try
        {
            auto& p = _get(eventName, pluginFileName);
            auto res = files::remove(p.path());
            _plugins[eventName].remove(p);
            if (!res)
                return ml::ret::fail("The plugin " + pluginFileName + " was not found to be removed.");
        }
        catch(const std::exception& e)
        {
            return ml::ret::fail("The plugin " + pluginFileName + " could not be uninstalled.\nMore infos : " + e.what());
        }
        return ml::ret::success();
    }
}
