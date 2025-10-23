#include "CmdPgr.h"
#include "mlprocess.h"
#include "os.h"
#include "files.2/files.h"
#include <stdexcept>

CmdPgr::CmdPgr()
{
    this->readConfig();
}

CmdPgr::CmdPgr(int argc, char* argv[])
{
    _argc = argc;
    _argv = argv;
    _name = std::string(argv[0]);
    _options = args::nparse(argc, argv);

    auto tmp = str::split(_name, files::sep());
    _name = vc::last(tmp);
    try
    {
        if (!files::isDir(files::configPath()))
            files::mkdir(files::configPath());
        _logFile = files::configPath() + files::sep() + _name + ".log";
    }

    catch(const std::exception& e)
    {
        lg("trying to create the logfile parentdir");
        std::cerr << e.what() << std::endl;
        _logFile = files::tmp() + files::sep() + _name + ".log";
    }

    lg2("logfile path", _logFile);
    lg("arguments parsed.");
}

CmdPgr::CmdPgr(std::map<std::string, std::string> options)
{
    _options = options;
    if (!files::isDir(files::configPath()))
        files::mkdir(files::configPath());

    _logFile = files::configPath() + files::sep() + "tmpname.log";
    lg2("logfile path", _logFile);
}

std::string CmdPgr::option(const std::string& key)
{
    if (_options.find(key) != _options.end())
        return _options[key];
    return "";
}

bool CmdPgr::optionExistsAndEmpty(const std::string& key)
{
    return (this->optionExists(key) && (_options[key].empty() || _options[key] == "-" + key));
}

std::map<std::string, std::string>& CmdPgr::options()
{
    return _options;
}
std::string CmdPgr::configPath()
{
    if (_name.empty())
        throw std::runtime_error("_name is empty.");
    if (_local)
        return files::execDir() + files::sep() + ".config" + files::sep() + _name;
    else 
        return os::home() + files::sep() + ".config" + files::sep() + _name;
}

std::string CmdPgr::config(const std::string& key, bool readFromFile)
{
    if (readFromFile)
        this->readConfig();
    if (_configs.find(key) != _configs.end())
        return _configs[key];
    return "";
}

std::string CmdPgr::execPath()
{
    if (_name.empty())
        throw std::runtime_error("_name is empty.");
    return files::execDir() + files::sep() + _name;
}

void CmdPgr::run()
{
    if (!this->option("version").empty())
        std::cout << this->version() << std::endl;
}

void CmdPgr::readConfig()
{
    if (_configReaded)
    {
        lg("config already readed. Abort.");
        return;
    }
    else 
        lg("reading config...");
    _configs.clear();

    if (!files::isDir(files::configPath()))
        return;

    for (auto &f : files::ls(files::configPath()))
    {
        if (files::isDir(f))
            lg(files::name(f) << " is a dir !");
        _configs[files::name(f)] = files::read(f);
    }

    lg("Config readed ! ");
    _configReaded = true;
}

void CmdPgr::reloadConfig()
{
    _configReaded = false;
    this->readConfig();
}

void CmdPgr::test()
{
    std::cout << "Testing " << _name << std::endl;
    std::cout << "Testing config file ..." << std::endl;
    this->setConfig("test", (std::string)"This is a test config.");
    this->saveConfig("test");

    std::string path = this->configPath() + files::sep() + "test";
    auto c = files::read(path);
    if (c == "This is a test config.")
        std::cout << "Config files work" << std::endl;
    else 
    {
        std::cout << "Error : content of ile is not the same" << std::endl;
        std::cout << "File exsist" << files::exists(path) << std::endl;
        std::cout << "File content" << files::read(path) << std::endl;
    }

    files::remove(path);
    _configs.erase("test");
}

void CmdPgr::saveConfig(const std::string& key)
{
    if (!key.empty())
    {
        this->_saveConfigKey(key);
        return;
    }

    for (std::map<std::string, std::string>::iterator it = _configs.begin(); it != _configs.end(); ++it)
        this->_saveConfigKey(it->first);
}

void CmdPgr::_saveConfigKey(const std::string &key)
{
    files::write(this->configPath() + files::sep() + key, this->config(key, false));
}

void CmdPgr::addToRecentFiles(const std::string& path)
{
    auto recentFiles = this->recentFiles();
    if (!recentFiles.includes(path))
        recentFiles.push(path);
    this->saveRecentFiles(recentFiles);
}

void CmdPgr::removeFromRecentFiles(const std::string& path)
{
    auto recentFiles = this->recentFiles();
    recentFiles.remove(path);
    this->saveRecentFiles(recentFiles);
}

ml::Vec<std::string> CmdPgr::recentFiles()
{
    auto recents = this->config("recent_files");
    ml::Vec<std::string> recentFiles;
    if (!recents.empty())
    {
        auto tmp = str::split(recents, "\n");
        for (const auto& s : tmp)
        {
            if (!s.empty())
                recentFiles.push(s);
        }
    }
    return recentFiles;

}
void CmdPgr::clearRecentFiles()
{
    this->setConfig("recent_files", std::string(""));
}

void CmdPgr::saveRecentFiles(const ml::Vec<std::string>& recents)
{
    std::string data = "";
    for (const auto& s : recents )
        data += s + "\n";
    this->setConfig("recent_files", data);
}

std::string CmdPgr::input(const std::string& msg) const
{
    std::string _r;
    std::cout << msg;
    std::getline(std::cin, _r);
    return _r;
}

namespace cmd 
{
    ml::Vec<std::string> filesFromArg(const std::string& arg)
    {
        return str::split(arg, ":");
    }
    std::string argfromFiles(const ml::Vec<std::string>& files)
    {
        std::string _r;
        for (const auto &s : files)
            _r += s + ":";
        _r.pop_back();
        return _r;
    }
}
