#include "./debug.h"
#include <iostream>
#ifdef NO_LOG
#else
#include "./files.2/files.h"
#include "./str.h"
#include <sys/types.h>
#include "./mlTime.h"
#include <mutex>
#include <cassert>
#endif

#ifdef __linux__
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif

using namespace std;
void db::log(const vector<string> &vec)
{
    for (auto s : vec)
        lg(s);
}

namespace db
{
    std::string _filepath;
#ifdef NO_LOG
#else 
    std::mutex _logfileMutex;
#endif
    void write(std::string tolog)
    {
#ifdef NO_LOG
#else
        std::lock_guard<std::mutex> lock(_logfileMutex);
        static int opened = 0;

        assert(_filepath.size() > 0 && "No log file set.");

        auto parent = files::parent(_filepath);
        auto parent_exists = files::isDir(parent);

        assert (parent_exists && "Directory does not exist for logging.");

        auto time = ml::time::asStringForFile(ml::time::time());
        auto tmp = str::split(time, "_");
        tolog = tmp[0] + " : " + str::replace(tmp[1], "-", ":") +  " => " + tolog;
        opened++;
        files::append(_filepath, tolog + "\n\n");
        opened--;
        assert(opened == 0);
#endif
    }

    void setLogFile(const std::string& path)
    {
#ifdef NO_LOG
#else
        std::lock_guard<std::mutex> lock(_logfileMutex);
        _filepath = path;
#endif
    }

    void setLogFile(const std::string& dirpath, char* programname)
    {
#ifdef NO_LOG
#else
        std::lock_guard<std::mutex> lock(_logfileMutex);
        _filepath = dirpath + files::sep() + std::string(programname) + ".log";
#endif
    }
}
