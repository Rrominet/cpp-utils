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
    if (onlyDay)
        return asString(time, _S"%d/%m/%Y");
    else 
        return asString(time, _S"%d/%m/%Y %H:%M:%S");
}

std::string ml::time::asString(int64_t time, const std::string& format)
{
    // Guard against values outside time_t range on this platform
    if (time < static_cast<std::int64_t>(std::numeric_limits<time_t>::min()) ||
        time > static_cast<std::int64_t>(std::numeric_limits<time_t>::max())) {
        lg("Time out of range : " << time);
        return "Unkown";
    }

    time_t raw = static_cast<time_t>(time);
    std::tm tm{};

#if defined(_WIN32)
    // Thread-safe on Windows
    if (localtime_s(&tm, &raw) != 0) {
        lg("Failed to get local time (windows)");
        return "Unkown";
    }
#else
    // Thread-safe on POSIX
    if (localtime_r(&raw, &tm) == nullptr) {
        lg("Failed to get local time (posix)");
        return "Unkown";
    }
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
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

std::string  ml::time::dateCleaned(std::string str)
{
    // Extract only digits from the input
    std::string digits;
    std::copy_if(str.begin(), str.end(), std::back_inserter(digits),
                 [](char c) { return std::isdigit(c); });
    
    // Pad with zeros if needed (default values)
    // Expected format: YYYYMMDD (8 digits)
    if (digits.length() < 8) {
        digits.resize(8, '0');
        // Set defaults: if less than 4 chars, pad year with leading zeros
        // if 4 chars (year only), add month 01 and day 01
        if (digits.length() == 4) {
            digits += "0101";
        } else if (digits.length() == 6) {
            // YYYYMM format, add day 01
            digits += "01";
        }
    }
    
    // Rebuild the string to ensure we have exactly YYYYMMDD
    std::string cleaned = digits.substr(0, 8);
    
    // Extract components
    std::string year = cleaned.substr(0, 4);
    std::string month = cleaned.substr(4, 2);
    std::string day = cleaned.substr(6, 2);
    
    // Handle defaults if month or day are 00
    if (month == "00") month = "01";
    if (day == "00") day = "01";
    
    // Format as YYYY-MM-DD
    return year + "-" + month + "-" + day;
}

std::string  ml::time::timeCleaned(std::string str)
{
    std::string digits;
    std::copy_if(str.begin(), str.end(), std::back_inserter(digits),
                 [](char c) { return std::isdigit(c); });
    
    // Pad with zeros if needed (default values: 00:00:00)
    // Expected format: HHMMSS (6 digits)
    if (digits.length() < 6) {
        digits.resize(6, '0');
    }
    
    // Take only first 6 digits
    std::string cleaned = digits.substr(0, 6);
    
    // Extract components
    std::string hours = cleaned.substr(0, 2);
    std::string minutes = cleaned.substr(2, 2);
    std::string seconds = cleaned.substr(4, 2);
    
    // Format as HH:mm:ss
    return hours + ":" + minutes + ":" + seconds;
}

std::string  ml::time::dateTimeCleaned(std::string str)
{
   // Split input on common separators or spaces to try to identify date vs time parts
    // Or just extract all digits and split them intelligently
    
    std::string digits;
    std::copy_if(str.begin(), str.end(), std::back_inserter(digits),
                 [](char c) { return std::isdigit(c); });
    
    // Assume first 8 digits are date (YYYYMMDD) and next 6 are time (HHMMSS)
    // Pad to 14 digits total if needed
    if (digits.length() < 14) {
        digits.resize(14, '0');
    }
    
    // Take first 14 digits
    std::string cleaned = digits.substr(0, 14);
    
    // Split into date and time parts
    std::string datePart = cleaned.substr(0, 8);
    std::string timePart = cleaned.substr(8, 6);
    
    // Use the previous functions
    std::string formattedDate = dateCleaned(datePart);
    std::string formattedTime = timeCleaned(timePart);
    
    // Combine with " - " separator
    return formattedDate + " - " + formattedTime;
}
