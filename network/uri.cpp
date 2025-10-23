#include "uri.h"
#include "str.h"
#include <map>
#include <iostream>

namespace uri
{
    std::map<std::string, std::string> refs;
    bool setted = false;
    void _setRefs()
    {
        if (setted)
            return;
        refs["%20"] = " ";
        refs["%21"] = "!";
        refs["%22"] = "\"";
        refs["%23"] = "#";
        refs["%24"] = "$";
        refs["%25"] = "%";
        refs["%26"] = "&";
        refs["%27"] = "'";
        refs["%28"] = "(";
        refs["%29"] = ")";
        refs["%2A"] = "*";
        refs["%2B"] = "+";
        refs["%2C"] = ",";
        refs["%2F"] = "/";
        refs["%3A"] = ":";
        refs["%3B"] = ";";
        refs["%3C"] = "<";
        refs["%3D"] = "=";
        refs["%3E"] = ">";
        refs["%3F"] = "?";
        refs["%40"] = "@";
        refs["%5B"] = "[";
        refs["%5C"] = "\\";
        refs["%5D"] = "]";
        refs["%5E"] = "^";
        refs["%5F"] = "_";
        refs["%60"] = "`";
        refs["%7B"] = "{";
        refs["%7C"] = "|";
        refs["%7D"] = "}";
        refs["%7E"] = "~";
        refs["%7F"] = " ";
        refs["%E2%82%AC"] = "€";
        refs["%81"] = "";
        refs["%E2%80%9A"] = "‚";
        refs["%C6%92"] = "ƒ";
        refs["%E2%80%9E"] = "„";
        refs["%E2%80%A6"] = "…";
        refs["%E2%80%A0"] = "†";
        refs["%E2%80%A1"] = "‡";
        refs["%CB%86"] = "ˆ";
        refs["%E2%80%B0"] = "‰";
        refs["%C5%A0"] = "Š";
        refs["%E2%80%B9"] = "‹";
        refs["%C5%92"] = "Œ";
        refs["%C5%8D"] = "";
        refs["%C5%BD"] = "Ž";
        refs["%8F"] = "";
        refs["%C2%90"] = "";
        refs["%E2%80%98"] = "‘";
        refs["%E2%80%99"] = "’";
        refs["%E2%80%9C"] = "“";
        refs["%E2%80%9D"] = "”";
        refs["%E2%80%A2"] = "•";
        refs["%E2%80%93"] = "–";
        refs["%E2%80%94"] = "—";
        refs["%CB%9C"] = "˜";
        refs["%E2%84"] = "™";
        refs["%C5%A1"] = "š";
        refs["%E2%80"] = "›";
        refs["%C5%93"] = "œ";
        refs["%9D"] = "";
        refs["%C5%BE"] = "ž";
        refs["%C5%B8"] = "Ÿ";
        refs["%C2%A0"] = " ";
        refs["%C2%A1"] = "¡";
        refs["%C2%A2"] = "¢";
        refs["%C2%A3"] = "£";
        refs["%C2%A4"] = "¤";
        refs["%C2%A5"] = "¥";
        refs["%C2%A6"] = "¦";
        refs["%C2%A7"] = "§";
        refs["%C2%A8"] = "¨";
        refs["%C2%A9"] = "©";
        refs["%C2%AA"] = "ª";
        refs["%C2%AB"] = "«";
        refs["%C2%AC"] = "¬";
        refs["%C2%AD"] = "­";
        refs["%C2%AE"] = "®";
        refs["%C2%AF"] = "¯";
        refs["%C2%B0"] = "°";
        refs["%C2%B1"] = "±";
        refs["%C2%B2"] = "²";
        refs["%C2%B3"] = "³";
        refs["%C2%B4"] = "´";
        refs["%C2%B5"] = "µ";
        refs["%C2%B6"] = "¶";
        refs["%C2%B7"] = "·";
        refs["%C2%B8"] = "¸";
        refs["%C2%B9"] = "¹";
        refs["%C2%BA"] = "º";
        refs["%C2%BB"] = "»";
        refs["%C2%BC"] = "¼";
        refs["%C2%BD"] = "½";
        refs["%C2%BE"] = "¾";
        refs["%C2%BF"] = "¿";
        refs["%C3%80"] = "À";
        refs["%C3%81"] = "Á";
        refs["%C3%82"] = "Â";
        refs["%C3%83"] = "Ã";
        refs["%C3%84"] = "Ä";
        refs["%C3%85"] = "Å";
        refs["%C3%86"] = "Æ";
        refs["%C3%87"] = "Ç";
        refs["%C3%88"] = "È";
        refs["%C3%89"] = "É";
        refs["%C3%8A"] = "Ê";
        refs["%C3%8B"] = "Ë";
        refs["%C3%8C"] = "Ì";
        refs["%C3%8D"] = "Í";
        refs["%C3%8E"] = "Î";
        refs["%C3%8F"] = "Ï";
        refs["%C3%90"] = "Ð";
        refs["%C3%91"] = "Ñ";
        refs["%C3%92"] = "Ò";
        refs["%C3%93"] = "Ó";
        refs["%C3%94"] = "Ô";
        refs["%C3%95"] = "Õ";
        refs["%C3%96"] = "Ö";
        refs["%C3%97"] = "×";
        refs["%C3%98"] = "Ø";
        refs["%C3%99"] = "Ù";
        refs["%C3%9A"] = "Ú";
        refs["%C3%9B"] = "Û";
        refs["%C3%9C"] = "Ü";
        refs["%C3%9D"] = "Ý";
        refs["%C3%9E"] = "Þ";
        refs["%C3%9F"] = "ß";
        refs["%C3%A0"] = "à";
        refs["%C3%A1"] = "á";
        refs["%C3%A2"] = "â";
        refs["%C3%A3"] = "ã";
        refs["%C3%A4"] = "ä";
        refs["%C3%A5"] = "å";
        refs["%C3%A6"] = "æ";
        refs["%C3%A7"] = "ç";
        refs["%C3%A8"] = "è";
        refs["%C3%A9"] = "é";
        refs["%C3%AA"] = "ê";
        refs["%C3%AB"] = "ë";
        refs["%C3%AC"] = "ì";
        refs["%C3%AD"] = "í";
        refs["%C3%AE"] = "î";
        refs["%C3%AF"] = "ï";
        refs["%C3%B0"] = "ð";
        refs["%C3%B1"] = "ñ";
        refs["%C3%B2"] = "ò";
        refs["%C3%B3"] = "ó";
        refs["%C3%B4"] = "ô";
        refs["%C3%B5"] = "õ";
        refs["%C3%B6"] = "ö";
        refs["%C3%B7"] = "÷";
        refs["%C3%B8"] = "ø";
        refs["%C3%B9"] = "ù";
        refs["%C3%BA"] = "ú";
        refs["%C3%BB"] = "û";
        refs["%C3%BC"] = "ü";
        refs["%C3%BD"] = "ý";
        refs["%C3%BE"] = "þ";
        refs["%C3%BF"] = "ÿ";

        setted = true;
    }

    std::string decode(std::string data)
    {
        _setRefs();
        for (auto const &[key, val] : refs)
            data = str::replace(data, key, val);
        return data;
    }

    std::string encode(std::string data)
    {
        _setRefs();
        for (auto const &[key, val] : refs)
            data = str::replace(data, val, key);
        return data;
    }

    std::string test()
    {
        auto res = uri::decode("https://teach.motion-live.com/formations/H3D2%20:%20Le%20Guide%20du%20Voyageur%203D/");

        std::cout << res << std::endl;
        return res;
    }
}
