#include "os.h"
#include "str.h"
#include <nlohmann/json.hpp>
#include <boost/process.hpp>
#include "./freedesktop.h"

using json = nlohmann::json;

#ifdef _WIN32
#pragma warning(disable : 4996) // need it to exec getenv on Window
#endif


std::string os::env(const std::string& envname)
{
    char const* val = std::getenv(envname.c_str()); 
    return val == NULL ? std::string() : std::string(val);
}

std::string os::type()
{
#ifdef __linux__
    return "linux";
#elif _WIN32
    return "windows";
#elif __APPLE__
    return "ios";
#endif
}
std::vector<std::string> os::appPaths()
{
#ifdef __linux__
    return freedesktop::desktopsPaths();
#else
    return std::vector<std::string>{};
#endif
}

std::vector<std::string> os::iconPaths()
{
#ifdef __linux__
    return freedesktop::iconPaths();
#else
    return std::vector<std::string>{};
#endif
}
std::string os::home()
{
#ifdef __linux__
    return os::lin::home();
#elif _WIN32
    return os::win::home();
#endif
}

#ifdef __linux__
std::string os::lin::home()
{
    return os::env("HOME");
}
#elif _WIN32 
std::string os::win::home()
{
    return os::env("HOMEDRIVE") + os::env("HOMEPATH"); // temp 
}
#endif

std::string os::clipboard()
{
#ifdef __linux__    
    std::string _r;
    boost::process::ipstream pipe_stream;
    boost::process::child child("xclip -selection clipboard -o", boost::process::std_out > pipe_stream);

    std::string line;

    while (child.running() && std::getline(pipe_stream, line) && !line.empty())
        _r += line + "\n";

    child.wait();
    return _r;
#else 
    throw std::runtime_error("Clipboard not supported on this platform : " + os::type());
#endif
    return "";
}

void os::setClipboard(std::string text)
{
#ifdef __linux__    
    text = str::replace(text, "'", "'\\''");
    std::string cmd = "echo -n '" + text + "' | xclip -selection clipboard";
    lg(cmd);
    boost::process::ipstream error_stream;
    int result = boost::process::system(cmd, boost::process::std_err > error_stream);
    if (result != 0)
        throw std::runtime_error("Failed to set clipboard, xclip error : " + std::to_string(result));
    std::cout << std::endl;
    std::string error_line;
    while(std::getline(error_stream, error_line)) {
        std::cerr << "Command error: " << error_line << std::endl;
    }
#else 
    throw std::runtime_error("Clipboard setting not supported on this platform : " + os::type());
#endif
}

