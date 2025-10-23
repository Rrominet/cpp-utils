#include "./freedesktop.h"
#include "./files.2/files.h"
#include "./gnome.h"
#include "./str.h"

namespace freedesktop
{
    std::vector<std::string> desktopsPaths()
    {
        return {
            "/usr/share/applications",
                "/usr/local/share/applications",
                os::home() + "/.local/share/applications",
                "/var/lib/snapd/desktop/applications",
        };
    }

    ml::Vec<std::unordered_map<std::string, std::string>> desktops()
    {
        ml::Vec<std::unordered_map<std::string, std::string>> _r;
        for (const auto& dir : desktopsPaths())
            _r.concat(desktopsFromDir(dir));
        return _r;
    }

    ml::Vec<std::unordered_map<std::string, std::string>> desktopsFromDir(const std::string& dir)
    {
        ml::Vec<std::unordered_map<std::string, std::string>> _r;
        for (const auto& filepath : files::ls(dir))
        {
            if (files::ext(filepath) == "desktop")
            {
                try
                {
                    _r.push_back(desktopFromFile(filepath));
                }
                catch(const std::exception& e)
                {
                    lg("Error trying to read and parse the file : " << filepath);
                    lg("Error : " << e.what());
                    continue;
                }
            }
        }
        
        return _r;
    }

    ml::Vec<std::string> desktopFiles()
    {
        ml::Vec<std::string> _r;
        for (const auto& dir : desktopsPaths())
            _r.concat(desktopFilesFromDir(dir));
        return _r;
    }

    ml::Vec<std::string> desktopFilesFromDir(const std::string& dir)
    {
        ml::Vec<std::string> _r;
        if (!files::isDir(dir))
            return _r;

        for (const auto& filepath : files::ls(dir))
        {
            if (files::ext(filepath) == "desktop")
                _r.push_back(filepath);
        }

        return _r;
    }

    std::unordered_map<std::string, std::string> desktopFromFile(const std::string& filepath)
    {
        std::string content = files::read(filepath);
        auto lines = str::split(content, "\n");
        std::unordered_map<std::string, std::string> _r;
        for (auto &l : lines)
        {
            if (str::contains(l, "[") && str::contains(l, "]"))
                continue;
            auto tmp = str::replace(l, " = ", "=");
            tmp = str::replace(l, "= ", "=");
            tmp = str::replace(l, " =", "=");

            auto table = str::split(tmp, "=");
            if (table.size() > 2)
            {
                std::string endpoint = table[1];
                for (int i = 2; i < table.size(); i++)
                    endpoint += "=" + table[i];
                table[1] = endpoint;
            }

            if (table.size() > 1)
            {
                if (_r.find(table[0]) == _r.end())
                    _r[table[0]] = table[1];
            }
        }

        return _r;
    }

    std::vector<std::string> iconPaths()
    {
        return {
            "/usr/share/pixmaps", 
                "/usr/share/icons",
                os::home() + "/.local/share/icons",
                os::home() + "/.local/share/pixmaps",
                "/var/lib/snapd/desktop/icons",
        };
    }

    std::string iconPathFromName(const std::string& name)
    {
        std::vector<std::string> exts = {"png", "svg", "jpg", "jpeg", "ico"};

        // searching if there is not a default one (like /usr/share/icons/name.png)
        for (const auto&s : iconPaths())
        {
            std::string path = s + "/" + name + ".";
            for (const auto&e : exts)
            {
                path += e;
                if (files::exists(path))
                    return path;
            }
        }

        std::string theme = gnome::icon_theme();
        if (theme == "")
            theme = "hicolor";

        std::string _r = iconPath(name, theme);
        if (_r == "")
            _r = iconPath(name, "hicolor");
        return _r;
    }

    std::string iconPath(const std::string& name, const std::string& theme)
    {
        std::vector<std::string> exts = {"png", "svg", "jpg", "jpeg", "ico"};
        // searching if a scalable could be found
        for (const auto&s : iconPaths())
        {
            std::string tmp = s + "/" + theme + "/scalable/apps/";
            if (!files::isDir(tmp))
                continue;
            for (const auto& path : files::ls(tmp))
            {
                if (name == files::baseName(path))
                    return path;
            }
        }

        // searching if any size could be found
        for (const auto&s : iconPaths())
        {
            std::string tmp = s + "/" + theme;
            if (!files::isDir(tmp))
                continue;
            for (const auto& dirname : files::ls(tmp, files::SortType::NAME, true))
            {
                std::string dirpath = tmp + "/" + dirname + "/apps";
                if (!files::isDir(dirpath))
                    continue;
                for (const auto& path : files::ls(dirpath))
                {
                    for (const auto&e : exts)
                    {
                        if (name + "." + e == files::name(path))
                            return path;
                    }
                }
            }
        }

        return "";
    }

    // remove the %f, %u, %n... from the command
    std::string execNoCodes(const std::string& cmd)
    {
        std::string _r = cmd;
        _r = str::replace(_r, "%f", "");
        _r = str::replace(_r, "%F", "");
        _r = str::replace(_r, "%u", "");
        _r = str::replace(_r, "%U", "");
        _r = str::replace(_r, "%d", "");
        _r = str::replace(_r, "%D", "");
        _r = str::replace(_r, "%n", "");
        _r = str::replace(_r, "%N", "");
        _r = str::replace(_r, "%i", "");
        _r = str::replace(_r, "%c", "");
        _r = str::replace(_r, "%k", "");
        _r = str::replace(_r, "%v", "");
        _r = str::replace(_r, "%m", "");
        return _r;
    }
}
