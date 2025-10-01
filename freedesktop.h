#pragma once
#include "os.h"
#include <unordered_map>
// this is a bunch of function that are usefull to work with freedesktop.org spec
//https://specifications.freedesktop.org/


namespace freedesktop
{
    std::vector<std::string> desktopsPaths();
    ml::Vec<std::unordered_map<std::string, std::string>> desktops();
    ml::Vec<std::unordered_map<std::string, std::string>> desktopsFromDir(const std::string& dir);

    ml::Vec<std::string> desktopFiles();
    ml::Vec<std::string> desktopFilesFromDir(const std::string& dir);
    std::unordered_map<std::string, std::string> desktopFromFile(const std::string& filepath);

    std::vector<std::string> iconPaths();
    std::string iconPathFromName(const std::string& name);
    std::string iconPath(const std::string& name, const std::string& theme);

    // remove the %f, %u, %n... from the command
    std::string execNoCodes(const std::string& cmd);

}
