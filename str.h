#pragma once

#include <locale>
#include <sys/types.h>
#include <vector>
#include <string>
#include <iostream>
#include <locale.h>
#include "vec.h"
#include "templates.h"

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
    
    std::string encode(const std::string& s);//nodify
    std::string decode(const std::string& s);//nodify

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
        Nb asNumber(const std::string& v)
        {
            if (!isANumber(v)) return 0;
            if constexpr (tpl::checkType<Nb, double>())
                return std::stod(v);
            else if constexpr (tpl::checkType<Nb, float>())
                return std::stof(v);
            else if constexpr (tpl::checkType<Nb, long>())
                return std::stol(v);
            else if constexpr (tpl::checkType<Nb, int>())
                return std::stoi(v);
            else
                return 0;
        }

    // carful here if the strings in the lists are deleted, the pointers in the retrunred char ** will not be ok anymore. use the 2nd function if you want to use the char** after the death of the vector.
    const char ** fromStringList(const std::vector<std::string>& vec);
    char ** fromStringListCopy(const std::vector<std::string>& vec);
}

namespace ml
{
    class String {
        public:
            // Constructors
            String();
            String(const char* str);
            String(const char c);
            String(const std::string& str);
            String(const String& other);
            String(bool val){str_ = str::fromBool(val);}
            String(double val){str_ = str::fromDouble(val);}
            String(int val){str_ = str::fromInt(val);}
            String(void* ptr){str_ = str::asString(ptr);}

            // Destructor
            virtual ~String(){}

            // Operators
            String& operator=(const String& other);
            String& operator=(const char* str);
            String& operator=(const std::string& str);
            String& operator+=(const String& other);
            String& operator+=(const char* str);
            String& operator+=(const std::string& str);
            friend String operator+(const String& lhs, const String& rhs);
            friend String operator+(const String& lhs, const char* rhs);
            friend String operator+(const char* lhs, const String& rhs);
            friend String operator+(const String& lhs, const std::string& rhs);
            friend String operator+(const std::string& lhs, const String& rhs);

            operator std::string() const{return str_;}
            operator std::string&() {return str_;}
            operator const std::string&() const {return str_;}

            char& operator[](size_t pos){return str_[pos];}
            const char& operator[] (size_t pos) const {return str_[pos];}

            // Methods
            const char* c_str() const;
            std::string str() const;
            std::string& str() {return str_;};
            std::string& s() {return str_;};
            std::string s() const {return str_;};
            bool empty() const;
            std::size_t size() const;
            void clear();
            String substr(std::size_t pos, std::size_t len) const;
            std::size_t find(const String& str, std::size_t pos = 0) const;
            std::size_t find(const char* str, std::size_t pos = 0) const;
            std::size_t find(char c, std::size_t pos = 0) const;
            void replace(std::size_t pos, std::size_t len, const String& str);
            void replace(std::size_t pos, std::size_t len, const char* str);
            void replace(std::size_t pos, std::size_t len, const std::string& str);

            ml::Vec<std::string> split(const String& delimiter, const String &exception = "") const {return ml::Vec(str::split(str_, delimiter, exception));}
            void remove(const String &toRemove){str_ = str::remove(str_, toRemove);}
            void replace(const String& search, const String& replace){str_ = str::replace(str_, search, replace);}
            String replaced(const String& search, const String& replace) const{return str::replace(str_, search, replace);}

            void lower(){str_ = str::lower(str_);} 
            void upper(){str_ = str::upper(str_);}

            String lowered() {return str::lower(str_);}
            String uppered() {return str::upper(str_);}

            void clean(const bool &removeMaj=false){str_ = str::clean(str_, removeMaj);}
            String cleaned(const bool &removeMaj=false) const{return str::clean(str_, removeMaj);};
            String email() const{return str::emailFromCleaned(str_);}

            char last() const{return str::last(str_);}
            bool contains(const String& searched) const {return str::contains(str_, searched);}

            String lastLine() const{return str::lastLine(str_);}
            void pop() {str_.pop_back();}

            int spaceBegining() const{return str::spaceBegining(str_);}

            String quoted()const {return str::quote(str_);}
            String unquoted()const {return str::unquote(str_);}

            String encoded()const {return str::encode(str_);}
            String decoded()const {return str::decode(str_);}

            void quote() {str_ = str::quote(str_);}
            void unquote(){str_ = str::unquote(str_);}

            void encode(){str_ = str::encode(str_);}
            void decode(){str_ = str::decode(str_);}

            void pad(char paddingChar = '0', int number=4){str_ = str::pad(str_, paddingChar, number);}
            String padded(char paddingChar = '0', int number=4)const {return str::pad(str_, paddingChar, number);}

            std::vector<std::string> in(char first, char second)const {return str::in(str_, first, second);}
            // if removeOuterSumbols it will also remove the first and second character given in argument 
            // ex first : <, second  :> my <String> -> my (and not my <>)
            void removeIn(char first, char second, bool removeOuterSumbols=false){str_ = str::removeIn(str_, first, second, removeOuterSumbols);}

            //return true if _in is between first and second in s
            bool isIn(const std::string& _in, char first, char second)const {return str::isIn(str_, _in, first, second);}
            //
            //return 0 if not founded in container
            int has(const String& searched)const {return str::has(str_, searched);}
            int has(char searched)const {return str::has(str_, String(searched));}
            int differences(const String& s2)const {return str::differences(str_, s2.str());}
            bool asBool()const {return str::asBool(str_);}
            double asDouble()const {return str::asDouble(str_);}
            int asInt()const {return str::asInt(str_);}
            bool isANumber()const {return str::isANumber(str_);}
            void capitalize(bool everyWord=true){str_ = str::capitalize(str_, everyWord);}
            String capitalized(bool everyWord=true) const {return str::capitalize(str_, everyWord);}

            void removeLast(const ml::String& sep);

        private:
            std::string str_;
    };

    bool operator==(const String& lhs, const String& rhs);
    bool operator!=(const String& lhs, const String& rhs);
    bool operator<(const String& lhs, const String& rhs);
    bool operator>(const String& lhs, const String& rhs);
    bool operator<=(const String& lhs, const String& rhs);
    bool operator>=(const String& lhs, const String& rhs);
}

namespace std
{
    std::string to_string(const std::string& v);
}
