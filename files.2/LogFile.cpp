#include "./LogFile.h"
#include <thread>
#include "./str.h"
#include "./mlTime.h"

namespace ml
{
    LogFile::LogFile(const std::string& filepath)
    {
        _filepath = filepath;
    }

    void LogFile::write(std::string tolog)
    {
        auto f = [this, tolog]() mutable
        {
            auto time = ml::time::asStringForFile(ml::time::time());
            auto tmp = str::split(time, "_");
            tolog = tmp[0] + " : " + str::replace(tmp[1], "-", ":") +  " => " + tolog + "\n\n";
            std::lock_guard<std::mutex> lock(_mutex);
            files::append(_filepath, tolog);
        };

        bool async = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            async = _async;
        }
        if (async)
            std::thread(f).detach();
        else
            f();
    }

    bool LogFile::async() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _async;
    }

    void LogFile::setAsync(bool async)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _async = async;
    }

    std::string LogFile::filepath() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _filepath;
    }

    void LogFile::setFilepath(const std::string& filepath)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _filepath = filepath;
    }
}
