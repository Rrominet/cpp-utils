#include "./str.h"
#include "./debug.h"
#include <string_view>
#include <time.h>
#include <algorithm>
#include <iomanip>
#include <cctype>
#include <sstream>
#include "./mlTime.h"
#include "./tconv.h"

using namespace std; 

namespace str
{
    int lastTime = 0;
}
std::vector<std::string> str::split(const std::string& str, const std::string& delimiter, const std::string &exception)
{
    if (str.empty())
        return std::vector<std::string>();

    std::vector<std::string> result;
    size_t pos = 0, start = 0, delim_len = delimiter.length();

    while ((pos = str.find(delimiter, start)) != std::string::npos) {
        result.emplace_back(str.substr(start, pos - start));
        start = pos + delim_len;
    }
    result.emplace_back(str.substr(start));

    if (exception.size() < 2)
        return result;

    std::vector<std::string> nres;
    size_t n = result.size();
    char exc0 = exception[0], exc1 = exception[1];

    for (size_t i = 0; i < n; ++i) {
        if (i == n - 1) {
            nres.push_back(result[i]);
            break;
        }
        if (str::contains(result[i], exc0) && !str::contains(result[i], exc1)) {
            std::string ns = result[i];
            int stage = str::has(ns, exc0) - str::has(ns, exc1);
            for (size_t j = i + 1; j < n; ++j) {
                stage += str::has(result[j], exc0);
                stage -= str::has(result[j], exc1);

                ns += delimiter + result[j];

                if (stage <= 0) {
                    i = j;
                    break;
                }
            }
            nres.push_back(ns);
        } else {
            nres.push_back(result[i]);
        }
    }

    return nres;
}

std::string str::remove(std::string str, const std::string &toRemove)
{
    size_t pos = std::string::npos;

    // Search for the substd::string in std::string in a loop untill nothing is found
    while ((pos = str.find(toRemove))!= std::string::npos)
    {
        // If found then erase it from std::string
        str.erase(pos, toRemove.length());
    }

    return str;
}

std::string str::random(size_t length, const int &seed)
{
    if (seed == -1)
    {
        auto seed = ml::time::mlnow() + rand()/10000;
        srand(seed);
    }
    else 
        srand(seed);
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    generate_n( str.begin(), length, randchar );
    return str;
}

std::string str::replace(const std::string& container, const std::string& search, const std::string& replace, u_int64_t max)
{
    if (container.empty())
        return container;
    if (search.empty())
        return container;

    size_t index = 0;
    std::string _r = container;
    int count = 0;
    while (true) {
        if (count >= max) break;
        /* Locate the substd::string to replace. */
        index = _r.find(search, index);
        if (index>= _r.size()) break;
        if (index == std::string::npos) break;

        /* Make the replacement. */
        _r.replace(index, search.length(), replace);

        /* Advance index forward so the next iteration doesn't pick it up as well. */
        index += replace.length();
        count++;
    }

    return _r;
}

std::string str::lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return s;
}

std::string str::upper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return s;
}

std::string str::capitalize(const std::string& s, bool everyWord)
{
    if (s.size() == 0)
        return "";
    std::string _r;
    if (everyWord)
    {
        auto tmp = str::split(s, " ");
        for (auto& w : tmp)
        {
            _r += str::capitalize(w, false);
            _r += " ";
        }

        _r.pop_back();
        return _r;
    }

    if (s.size() == 0)
        return "";
    _r += toupper(s[0]);
    if (s.size() == 1)
        return _r;
    for (int i=1; i<s.size(); i++)
        _r += s[i];
    return _r;
}

std::string str::clean(std::string s, const bool &removeMaj)
{
    if (removeMaj)
        s = str::lower(s);
    s = str::replace(s, "é", "e");
    s = str::replace(s, "è", "e");
    s = str::replace(s, "ë", "e");
    s = str::replace(s, "ê", "e");
    s = str::replace(s, "à", "a");
    s = str::replace(s, "ä", "a");
    s = str::replace(s, "â", "a");
    s = str::replace(s, "ü", "u");
    s = str::replace(s, "û", "u");
    s = str::replace(s, "î", "i");
    s = str::replace(s, "ï", "i");
    s = str::replace(s, " ", "-");
    s = str::replace(s, "@", "_a_");
    s = str::replace(s, "{", "_");
    s = str::replace(s, "(", "_");
    s = str::replace(s, "}", "_");
    s = str::replace(s, ")", "_");
    s = str::replace(s, "]", "_");
    s = str::replace(s, "[", "_");
    s = str::replace(s, "&", "_");
    s = str::replace(s, "[", "_");
    s = str::replace(s, "|", "_");
    s = str::replace(s, "#", "_");
    s = str::replace(s, "\"", "");
    s = str::replace(s, "=", "");
    s = str::replace(s, "+", "");
    s = str::replace(s, "%", "");
    s = str::replace(s, "\\", "_");
    s = str::replace(s, "/", "_");
    s = str::replace(s, "ç", "c");
    return s;
}

std::string str::emailFromCleaned(std::string cleaned)
{
    return str::replace(cleaned, "_a_", "@");
}

char str::last(const std::string& s)
{
    return s[s.size()-1];
}

bool str::contains(const std::string& container, const std::string& searched)
{
    return container.find(searched) != std::string::npos;
}

bool str::contains(const std::string& container, char searched)
{
    return container.find(searched) != std::string::npos;
}

std::string str::lastLine(std::string s)
{
    if (str::last(s) == '\n')
        s.pop_back();
    for(size_t i = s.size() - 1; i>=0; i--)
    {
        if (s[i] == '\n')
            return s.substr(i+1);
    }
    return "";
}
std::string str::join(const std::vector<std::string>& vec, const std::string& join)
{
    std::string _r="";
    for (int i=0; i<vec.size(); i++)
    {
        if (i != vec.size() - 1)
            _r += vec[i] + join;
        else 
            _r += vec[i];
    }

    return _r;
}

int str::spaceBegining(std::string s)
{
    int spaces = 0;
    if (s.empty())
        return spaces;
    while (s[0] == ' ')
    {
        spaces ++;
        s = s.substr(1);
    }
    return spaces;
}

std::string str::quote( const std::string& s )
{
    std::ostringstream ss;
    ss << std::quoted( s );
    return ss.str();
}

std::string str::unquote( const std::string& s )
{
    std::string result;
    std::istringstream ss( s );
    ss >> std::quoted( result );
    return result;
}

std::string str::encode(const std::string& s, int offset, bool ignoreNonVisible)
{
    std::string out;
    out.reserve(s.size());

    const int minChar = ignoreNonVisible ? 33 : 32;
    const int maxChar = 126;
    const int range = maxChar - minChar + 1;

    for (unsigned char c : s)
    {
        if (ignoreNonVisible)
        {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
                out.push_back(c);
                continue;
            }
        }

        if (c < minChar || c > maxChar)
        {
            out.push_back(c);
            continue;
        }

        int normalized = c - minChar;
        int shifted = (normalized + offset) % range;

        if (shifted < 0)
            shifted += range;

        out.push_back(static_cast<char>(minChar + shifted));
    }

    return out;
}

std::string str::decode(const std::string& s, int offset, bool ignoreNonVisible)
{
    return str::encode(s, -offset, ignoreNonVisible);
}

void str::test::encode()
{
    auto tst = "http://111.90.157.68/~blopfrog";
    std::cout << tst << " is encoded in " << str::encode(tst) << std::endl;
}

void str::test::decode()
{
    auto tst = "iuuq;00222/:1/268/790cmpqgsph";
    std::cout << tst << " is decoded in " << str::decode(tst) << std::endl;

}

void str::test::enc_dec()
{
    std::string orig = "http://111.90.157.68/~blopfrog";
    std::string enc = str::encode(orig);
    std::string decode = str::decode(enc);

    std::cout << "encoded : " << enc << std::endl;
    std::cout << "decoded : " << decode << std::endl;

    if (orig == decode)
        std::cout << "Decoding and encoding works" << std::endl;
    else 
    {
        std::cerr << "Didn't work" << std::endl;

        std::cout << "orig size : " << orig.size() << std::endl;
        std::cout << "decoded size : " << decode.size();
    }
}

std::string str::pad(std::string s, char paddingChar , int number)
{
    if (number>s.size())
        s.insert(0, number - s.size(), paddingChar);
    return s;
}

std::string str::pad(int nb, char paddingChar, int number)
{
    return str::pad(std::to_string(nb), paddingChar, number);
}

std::vector<std::string> str::in(const std::string& s, char first, char second)
{
    std::vector<int> ins;
    std::vector<int> outs;
    std::vector<std::string> rs;

    for (int i=0; i<s.size(); i++)
    {
        if (s[i] == first) 
            ins.push_back(i);
        else if (s[i] == second)
            outs.push_back(i);
    }

    if (ins.size() == 0 || outs.size() == 0)
        return rs;

    while (outs[0]<=ins[0])
    {
        outs.erase(outs.begin());
        if (outs.size() == 0)
            return rs;
    }

    for (int i=0; i<ins.size(); i++)
    {
        if (i>= outs.size())
            break;
        if (ins[i] + 1 >= s.size())
            break;

        std::string ns = s.substr(ins[i]+1, outs[i] - ins[i] - 1);
        rs.push_back(ns);
    }

    return rs;
}

std::string str::removeIn(std::string s, char first, char second, bool removeOuterSumbols)
{
    auto ins = str::in(s, first, second);
    std::string sf; sf.push_back(first); sf.push_back(second);
    for (auto &i : ins)
    {
        if (removeOuterSumbols)
            s = str::replace(s, first + i + second, "");
        else 
            s = str::replace(s, first + i + second, sf);
    }

    return s;
}

bool str::isIn(const std::string& s, const std::string& _in, char first, char second)
{
    auto ins = str::in(s, first, second);
    return vc::contains(ins, _in);
}

int str::has(std::string container, const std::string& searched)
{
    int _r = 0;
    while (true)
    {
        auto founded = container.find(searched);
        if (founded == std::string::npos)
            break;
        _r ++;

        container = container.substr(founded + 1);
    }

    return _r;
}

int str::has(const std::string &container, char searched)
{
    return std::count(container.begin(), container.end(), searched);
}

int str::differences(const std::string& s1, const std::string& s2)
{
    int differences = 0;
    for (int i=0; i<s1.size(); i++)
    {
        if (s2.size()<=i)
            break;

        if (s1[i] != s2[i])
            differences ++;
    }

    differences += abs((int)(s1.size() - s2.size()));

    return differences;
}

bool str::asBool(const std::string& v)
{
    return tconv::convert<std::string, bool>(v);
}

std::string str::fromBool(bool v)
{
    return tconv::convert<bool, std::string>(v);
}

float str::asFloat(const std::string& v)
{
    return tconv::convert<std::string, float>(v);
}
    std::string str::fromFloat(float v)
{
    return tconv::convert<float, std::string>(v);
}

double str::asDouble(const std::string& v)
{
    return tconv::convert<std::string, double>(v);
}

std::string str::fromDouble(double v)
{
    return tconv::convert<double, std::string>(v);
}

int str::asInt(const std::string& v)
{
    return tconv::convert<std::string, int>(v);
}

std::string str::fromInt(int v)
{
    return tconv::convert<int, std::string>(v);
}

long str::asLong(const std::string& v)
{
    return tconv::convert<std::string, long>(v);
}

std::string str::fromLong(long v)
{
    return tconv::convert<long, std::string>(v);
}

bool str::isANumber(const std::string& v)
{
    try
    {
        stoi(v);
        return true;
    }catch(...){return false;}

    try
    {
        stod(v);
        return true;
    }catch(...){return false;}
    return false;
}

namespace str
{
    std::string majs = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
}

bool str::isAMaj(const std::string& v)
{
    return str::contains(str::majs, v);
}

std::string str::asString(void* ptr)
{
    std::ostringstream address;
    address << ptr;
    return address.str();
}
std::pair<uint32_t, uint32_t> str::unicodeRangeForLanguage(Language language) {
    switch(language) {
        case Language::EN:
            // Basic Latin (ASCII) and Latin-1 Supplement
            return {0x0000, 0x00FF};
        case Language::FR:
            // Basic Latin (ASCII), Latin-1 Supplement, and some additional French characters in Latin Extended-B
            return {0x0000, 0x024F};
        case Language::ES:
            // Basic Latin (ASCII), Latin-1 Supplement
            return {0x0000, 0x00FF};
        case Language::DE:
            // Basic Latin (ASCII), Latin-1 Supplement
            return {0x0000, 0x00FF};
        default:
            throw std::invalid_argument("Unsupported language");
    }
}

    // carful here if the strings in the lists are deleted, the pointers in the retrunred char ** will not be ok anymore. use the 2nd function if you want to use the char** after the death of the vector.
const char** str::fromStringList(const std::vector<std::string>& vec)
{
    const char** r = new const char*[vec.size() + 1];
    for (int i=0; i<vec.size(); i++)
        r[i] = vec[i].c_str();
    r[vec.size()] = NULL;
    return r;
}

char** str::fromStringListCopy(const std::vector<std::string>& vec)
{
    char** r = new char*[vec.size() + 1];
    for (int i=0; i<vec.size(); i++)
    {
        r[i] = new char[vec[i].size() + 1];
#ifdef _WIN32
        strcpy_s(r[i], vec[i].size() + 1, vec[i].c_str());
#elif __linux__
        strcpy(r[i], vec[i].c_str());
#endif
    }
    r[vec.size()] = NULL;
    return r;
}

bool str::startsWith(const std::string& s, const std::string& start, bool trimWithSpaces)
{
    if (start.size() > s.size())
        return false;
    std::string s2 = "";
    if (!trimWithSpaces)
        s2 = s;
    else
    {
        for (int i=0; i<s.size(); i++)
        {
            if (s[i] != ' ')
                s2 += s[i];
        }
    }
    return s2.compare(0, start.size(), start) == 0;
}

namespace str
{
     bool isAsciiSpace(char character)
    {
        return character == ' '  ||
               character == '\t' ||
               character == '\n' ||
               character == '\r' ||
               character == '\f' ||
               character == '\v';
    }

     std::string_view trim(
        std::string_view text)
    {
        while (!text.empty() && isAsciiSpace(text.front()))
            text.remove_prefix(1);

        while (!text.empty() && isAsciiSpace(text.back()))
            text.remove_suffix(1);

        return text;
    }

     char asciiLower(char character)
    {
        if (character >= 'A' && character <= 'Z')
        {
            return static_cast<char>(
                character - 'A' + 'a');
        }

        return character;
    }

     bool equalsIgnoreCase(
        std::string_view left,
        std::string_view right)
    {
        if (left.size() != right.size())
            return false;

        for (std::size_t i = 0; i < left.size(); ++i)
        {
            if (asciiLower(left[i]) != asciiLower(right[i]))
                return false;
        }

        return true;
    }

    bool isEqual(const std::string& s1, const std::string& s2, bool ignoreCase)
    {
        if (ignoreCase)
            return equalsIgnoreCase(s1, s2);
        else
            return s1 == s2;
    }

    std::string trimed(const std::string& s)
    {
        std::string_view sv(s);
        return std::string(trim(sv));
    }
}

namespace std
{
    std::string to_string(const std::string& v){return v;}
}
