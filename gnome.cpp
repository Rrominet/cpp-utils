#include "./gnome.h"
#include "./mlprocess.h"

namespace gnome
{
    std::string _icon_name = "";
    std::string icon_theme()
    {
        if (!_icon_name.empty())
            return _icon_name;
        _icon_name = process::exec("gsettings get org.gnome.desktop.interface icon-theme");
        return _icon_name;
    }
}
