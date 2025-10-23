#pragma once
#include <iostream>
#include "./mlTime.h"
#include <string> 
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ml
{
    class Date
    {
        public : 
            Date(){}
            Date(int64_t time) : _time(time) {}
            Date(const std::string& time, const std::string& format = "%Y-%m-%d");

            virtual ~Date() = default;

            std::string asString(const std::string& format = "%Y-%m-%d") const;
            std::string asFrench()const {return this->asString("%d/%m/%Y");} 
            std::string timeAsFrench(bool includesSeconds=false)const;

            bool operator==(const Date& other) const;
            bool operator!=(const Date& other) const;
            bool operator<(const Date& other) const;
            bool operator>(const Date& other) const;
            bool operator<=(const Date& other) const;
            bool operator>=(const Date& other) const;

            Date operator+(const Date& other) const;
            Date operator-(const Date& other) const;

            friend std::ostream& operator<<(std::ostream& os, const Date& date);

            bool isInvalid() const;

            virtual json serialize() const;
            virtual void deserialize(const json& _j);

            void offsetBySeconds(int64_t seconds);
            void offsetByMinutes(double minutes);
            void offsetByHours(double hours);
            void offsetByDays(double days);
            void offsetByWeeks(double weeks);

            //precision in seconds
            bool isEqual(const Date& other, int64_t precision=0) const;

        private : 
            int64_t _time; //bp cgs
                           
        public :
#include "./Date_gen.h"
    };

    namespace tz
    {
        //the offset is in minutes
        int utc_offset(const std::string& timezone="Europe/Paris");
    }
}

namespace std
{
    // to make a Date hasable and be in a map
    template<>
        struct hash<ml::Date>
        {
            std::size_t operator()(const ml::Date& date) const noexcept
            {
                return std::hash<int64_t>()(date.time());
            }
        };
}
