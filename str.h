#pragma once

#include <locale>
#include <sys/types.h>
#include <vector>
#include <string>
#include <iostream>
#include <locale.h>
#include <string_view>
#include "./vec.h"
#include "./templates.h"

#define _S (std::string)

namespace str
{
    //if exception is defined, is a string with size of 2. 
    //for example "<>"
    //if delimiter is between exception[0] and exception[1], ths split will not be executed.
    std::vector<std::string> split(const std::string& str, const std::string& delimiter, const std::string &exception = ""); //nodify
    std::string remove(std::string str, const std::string &toRemove); //nodify
    std::string random(size_t length, const int &seed=-1); //nodify
    std::string replace(const std::string& container, const std::string& search, const std::string& replace, u_int64_t max=18446744073709551615ULL);//nodify

    std::string lower(std::string s); //nodify
    std::string upper(std::string s);//nodify

    std::string clean(std::string s, const bool &removeMaj=false);//nodify
    std::string emailFromCleaned(std::string cleaned);//nodify

    char last(const std::string& s);//nodify
    bool contains(const std::string& container, const std::string& searched);//nodify
    bool contains(const std::string& container, char searched);//nodify

    std::string lastLine(std::string s);//nodify
    std::string join (const std::vector<std::string>& vec, const std::string& join="");//nodify

    int spaceBegining(std::string s);//nodify

    std::string quote( const std::string& s );//nodify
    std::string unquote( const std::string& s );//nodify
    
    std::string encode(const std::string& s, int offset=1, bool ignoreNonVisible=false);//nodify
    std::string decode(const std::string& s, int offset=1, bool ignoreNonVisible=false);//nodify

    namespace test
    {
        void encode();
        void decode();

        void enc_dec();
    }

    // s need to be a number like "25"
    std::string pad(std::string s, char paddingChar = '0', int number=4);//nodify
    std::string pad(int nb, char paddingChar = '0', int number=5);//nodify
    std::vector<std::string> in(const std::string& s, char first, char second);//nodify
    // if removeOuterSumbols it will also remove the first and second character given in argument 
    // ex first : <, second  :> my <std::string> -> my (and not my <>)
    std::string removeIn(std::string s, char first, char second, bool removeOuterSumbols=false);//nodify

    //return true if _in is between first and second in s
    bool isIn(const std::string& s, const std::string& _in, char first, char second);//nodify
    //
    //return 0 if not founded in container
    int has(std::string container, const std::string& searched);//nodify
    int has(const std::string &container, char searched);//nodify
    int differences(const std::string& s1, const std::string& s2);//nodify

    bool asBool(const std::string& v);//nodify
    std::string fromBool(bool v);//nodify

    float asFloat(const std::string& v);//nodify
    std::string fromFloat(float v);//nodify

    double asDouble(const std::string& v);//nodify
    std::string fromDouble(double v);//nodify

    int asInt(const std::string& v);//nodify
    std::string fromInt(int v);//nodify

    long asLong(const std::string& v);//nodify
    std::string fromLong(long v);//nodify

    bool isANumber(const std::string& v);//nodify

    // work with only one letter.
    bool isAMaj(const std::string& v);

    std::string capitalize(const std::string& s, bool everyWord=true);//nodify

    std::string asString(void* ptr);

    enum Language {EN, FR, ES, DE};
    std::pair<uint32_t, uint32_t> unicodeRangeForLanguage(Language language);

    template <typename Nb>
        Nb asNumber(const std::string& v);

    // carful here if the strings in the lists are deleted, the pointers in the retrunred char ** will not be ok anymore. use the 2nd function if you want to use the char** after the death of the vector.
    const char ** fromStringList(const std::vector<std::string>& vec);
    char ** fromStringListCopy(const std::vector<std::string>& vec);
    bool startsWith(const std::string& s, const std::string& start, bool trimWithSpaces=true);

    bool isAsciiSpace(char character);

    std::string_view trim(
        std::string_view text);

    char asciiLower(char character);

    bool equalsIgnoreCase(
        std::string_view left,
        std::string_view right);

    bool isEqual(const std::string& s1, const std::string& s2, bool ignoreCase=false);
    std::string trimed(const std::string& s);

}

namespace std
{
    std::string to_string(const std::string& v);
}
