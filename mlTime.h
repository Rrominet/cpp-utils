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

        // foramt cleaned up for a file name
        std::string asStringForFile(int64_t time);

        // this the format returned by this function : yyyy-mm-dd
        std::string asStringReverse(int64_t time);

        // you need to put a % before your letters like that : %m-%d-%Y instead of m-d-Y
        int64_t fromString(const std::string& time, const std::string& format = "%d/%m/%Y %H:%M:%S");

        // if return value is negative, date < dateB, if 0, date == dateB, else date > dateB
        int compare(const std::string& dateA, const std::string& dateB, const std::string& format = "%Y-%m-%d");
        //return the number of seconds in double
        // time is in form HH:mm:ss
        double asSeconds(const std::string& time);
        //return the time as the form HH:mm:ss
        std::string asVideoTime(double seconds);
    }
}

