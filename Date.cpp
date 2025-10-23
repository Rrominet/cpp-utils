#include "./Date.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include "debug.h"

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable : 4996) // need it to exec localtime on Window

#endif

namespace ml
{
    Date::Date(const std::string& time, const std::string& format )
    {
        _time = ml::time::fromString(time, format);
    }

    std::string Date::asString(const std::string& format) const
    {
        time_t raw_time = static_cast<time_t>(_time);
        auto tm = *std::localtime(&raw_time);
        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());
        return oss.str();
    }

    bool Date::operator==(const Date& other) const
    {
        return this->_time == other._time;
    }

    bool Date::isEqual(const Date& other,int64_t precision) const
    {
        return std::abs(this->_time - other._time) < precision;
    }

    bool Date::operator!=(const Date& other) const
    {
        return this->_time != other._time;
    }
    bool Date::operator<(const Date& other) const
    {
        return this->_time < other._time;
    }
    bool Date::operator>(const Date& other) const
    {
        return this->_time > other._time;
    }
    bool Date::operator<=(const Date& other) const
    {
        return this->_time <= other._time;
    }
    bool Date::operator>=(const Date& other) const
    {
        return this->_time >= other._time;
    }

    Date Date::operator+(const Date& other) const
    {
        return Date(this->_time + other._time);
    }

    Date Date::operator-(const Date& other) const
    {
        return Date(this->_time - other._time);
    }

    std::ostream& operator<<(std::ostream& os, const Date& date)
    {
        os << date.asString("%Y-%m-%d : %H:%M:%S");
        return os;
    }

    bool Date::isInvalid() const
    {
        return _time <= 0;
    }

    json Date::serialize() const
    {
        json _r;
        _r["time"] = _time;
        return _r;
    }

    void Date::deserialize(const json& _j)
    {
        if (_j.contains("time"))
            _time = _j["time"];
    }

    void Date::offsetBySeconds(int64_t seconds)
    {
        _time += seconds;
    }


    void Date::offsetByMinutes(double minutes)
    {
        _time += minutes * 60;
    }


    void Date::offsetByHours(double hours)
    {
        _time += hours * 60 * 60;
    }


    void Date::offsetByDays(double days)
    {
        _time += days * 60 * 60 * 24;
    }


    void Date::offsetByWeeks(double weeks)
    {
        _time += weeks * 60 * 60 * 24 * 7;
    }

    std::string Date::timeAsFrench(bool includesSeconds) const
    {
        if (includesSeconds)
            return this->asString("%H:%M:%S");
        else
            return this->asString("%H:%M");
    }

    namespace tz
    {
        bool is_dst_now() 
        {
            std::time_t now = std::time(nullptr);
            std::tm* local_time = std::localtime(&now);
            return local_time->tm_isdst > 0;
        }

        int utc_offset(const std::string& tz_name)
        {
            if (tz_name == "UTC")
                return 0;
            else if (tz_name == "Europe/Paris")
            {
                if (!is_dst_now())
                    return 60;
                else 
                    return 120;
            }
            else
                throw std::invalid_argument("Unknown timezone : " + tz_name);
        }
    }

}

