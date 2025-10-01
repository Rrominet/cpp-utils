#include "apache.h"
#include "mlTime.h"
#include "debug.h"
#include "str.h"
#include "network/network.h"
#include "files.2/files.h"

namespace apache {
    std::string _ip = "not_setted";

    void overrideIp(const std::string& ip)
    {
        lg2("Overriding IP with", ip);
        apache::_ip = ip;
        lg("IP overrided.");
    }
}

std::string apache::ip()
{
    if (apache::_ip != "not_setted")
    {
        lg2("IP overrided", apache::_ip);
        return apache::_ip;
    }
    if (std::getenv("REMOTE_ADDR"))
        return std::getenv("REMOTE_ADDR");
    else 
        return "unkown";
}

void apache::Connection::open(const std::string &dir)
{
    if (!files::isDir(dir))
        files::mkdir(dir);

    _ip = apache::ip();
    _filepath = dir + files::sep() + _ip;
    int nb = 0;
    if (files::exists(_filepath))
    {
        auto data = files::read(_filepath);
        nb = stoi(data);
    }

    nb ++;
    files::write(_filepath, std::to_string(nb));
    _oppened = true;
}

void apache::Connection::close()
{
   if (!_oppened) 
       return;

   if (!files::exists(_filepath))
       return;

   auto data = files::read(_filepath);
   int nb = stoi(data);
   nb --;
   files::write(_filepath, std::to_string(nb));
   _oppened = false;
}

bool apache::reset(const std::string& dir, const int minTime)
{
    std::string pth = dir + files::sep() + apache::ip();
    if (!files::exists(pth))
        return true;
    
    long modified = files::lastTimeModified(pth)/1000;
    if (modified<ml::time::time() - minTime)
    {
        files::remove(pth);
        return true;
    }
    return false;
}

int apache::connections(const std::string &dir)
{
    apache::reset(dir, 600);
    std::string pth = dir + files::sep() + apache::ip();
    if (!files::exists(pth))
        return 0;

    return stoi(files::read(pth));
}

bool apache::log()
{
    std::string toLog;
    toLog = apache::ip() + " connected at : " + ml::time::asString(ml::time::time()) ;

    lg(toLog);
    std::string tmp_path = "/media/romain/Donnees/TEMP/apache";
    if (!files::isDir(tmp_path))
        tmp_path = "/var/www/html/motion-live/logs/apache";

    return files::append(tmp_path + files::sep() + "tchat-logs_" + apache::ip(), toLog + "\n");
}

bool apache::logout()
{
    std::string toLog;
    toLog = apache::ip() + " disconnected at : " + ml::time::asString(ml::time::time());

    lg(toLog);
    std::string tmp_path = "/media/romain/Donnees/TEMP/apache";
    if (!files::isDir(tmp_path))
        tmp_path = "/var/www/html/motion-live/logs/apache";
    return files::append(tmp_path + files::sep() + "tchat-logs_" + apache::ip(), toLog + "\n");
}

std::unordered_map<std::string, std::string> apache::GET()
{
    return network::get();
}
