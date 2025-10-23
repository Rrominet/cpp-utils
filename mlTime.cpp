#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "./debug.h"
#include "./mlTime.h"
#include "./str.h"
#include <chrono>
#include <string>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable : 4996) // need it to exec localtime on Window

#endif

using namespace std::chrono;

int64_t ml::time::time()
{
    return std::time(nullptr);
}

int64_t ml::time::now()
{
    return ml::time::time();
}

int64_t ml::time::mlnow()
{
    return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
            ).count();
}

std::string ml::time::asString(int64_t time, bool onlyDay)
{
    time_t raw_time = static_cast<time_t>(time);
    auto tm = *std::localtime(&raw_time);
    std::ostringstream oss;
    if (onlyDay)
        oss << std::put_time(&tm, "%d/%m/%Y");
    else 
        oss << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");
    return oss.str();
}

std::string ml::time::asStringForFile(int64_t time)
{
    auto tm = *std::localtime((const time_t*)&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

std::string ml::time::asStringReverse(int64_t time, bool includeHours )
{
    auto tm = *std::localtime((const time_t*)&time);
    std::ostringstream oss;
    if (!includeHours)
        oss << std::put_time(&tm, "%Y-%m-%d");
    else 
        oss << std::put_time(&tm, "%Y-%m-%d : %H:%M:%S");
    return oss.str();
}

int64_t  ml::time::fromString(const std::string& time, const std::string& format )
{
    std::istringstream iss(time);
    std::tm tm = {};
    iss >> std::get_time(&tm, format.c_str());

    if (iss.fail()) {
        // Handle error appropriately, e.g., throw an exception or return an error code
        throw std::runtime_error("Failed to parse time string");
    }

    // Convert tm to time_t and then to int64_t
    return static_cast<int64_t>(mktime(&tm));
}

int ml::time::compare(const std::string& dateA,const std::string& dateB,const std::string& format)
{
    auto u_dataA = ml::time::fromString(dateA, format);	
    auto u_dataB = ml::time::fromString(dateB, format);
    return u_dataA - u_dataB;
}

double ml::time::asSeconds(const std::string& time)
{
    std::regex pattern(R"((\d+):(\d{2}):(\d{2})(\.\d+)?)");
    std::smatch matches;

    if (std::regex_match(time, matches, pattern) && matches.size() >= 4) {
        int hours = std::stoi(matches[1].str());
        int minutes = std::stoi(matches[2].str());
        int seconds = std::stoi(matches[3].str());
        double fractional = 0.0;

        if (matches.size() == 5 && matches[4].matched) {
            fractional = str::asDouble(matches[4].str());
        }

        return hours * 3600.0 + minutes * 60.0 + seconds + fractional;
    }

    return 0.0; // Return 0 if time format is invalid
}

// Return the time as the form HH:mm:ss
std::string ml::time::asVideoTime(double totalSeconds)
{
    int hours = static_cast<int>(totalSeconds / 3600);
    int minutes = static_cast<int>((totalSeconds - hours * 3600) / 60);
    int seconds = static_cast<int>(totalSeconds) % 60;
    int milliseconds = static_cast<int>(std::round((totalSeconds - static_cast<int>(totalSeconds)) * 1000));

    std::ostringstream timeStream;
    timeStream << std::setfill('0') 
               << std::setw(2) << hours << ":"
               << std::setw(2) << minutes << ":"
               << std::setw(2) << seconds << "."
               << std::setw(3) << milliseconds;

    return timeStream.str();
}
