#pragma once 
#include <cstdint>
#include <string>
namespace ml
{
    namespace time
    {
        // in seconds from 1970
        int64_t time();
        int64_t now();

        // in miliseconds
        int64_t mlnow();

        // time NEED TO BE IN SECONDS, unless segfault ! 
        // readable format
        std::string asString(int64_t time, bool onlyDay=false);
        std::string asString(int64_t time, const std::string& format);

        // foramt cleaned up for a file name
        std::string asStringForFile(int64_t time);

        // this the format returned by this function : yyyy-mm-dd
        std::string asStringReverse(int64_t time, bool includeHours = false);

        // you need to put a % before your letters like that : %m-%d-%Y instead of m-d-Y
        int64_t fromString(const std::string& time, const std::string& format = "%d/%m/%Y %H:%M:%S");

        // if return value is negative, date < dateB, if 0, date == dateB, else date > dateB
        int compare(const std::string& dateA, const std::string& dateB, const std::string& format = "%Y-%m-%d");
        //return the number of seconds in double
        // time is in form HH:mm:ss
        double asSeconds(const std::string& time);
        //return the time as the form HH:mm:ss
        std::string asVideoTime(double seconds);

        //return a string at the format YYYY-MM-DD from what is in str
        //useful to constraint the input of a user for example
        std::string dateCleaned(std::string str);
        std::string timeCleaned(std::string str);
        std::string dateTimeCleaned(std::string str);
    }
}

