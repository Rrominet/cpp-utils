#pragma once
#include "./Date.h"
#include <cstdint>
#include <string> 

namespace ml
{
    class Period
    {
        public : 
            Period() : _start(0), _end(0){}
            Period(const Date& start, const Date& end) : _start(start), _end(end){}
            virtual ~Period() = default;

            int64_t duration() const;

            bool operator==(const Period& other) const;
            bool operator!=(const Period& other) const;
            bool operator<(const Period& other) const;
            bool operator>(const Period& other) const;
            bool operator<=(const Period& other) const;
            bool operator>=(const Period& other) const;

            virtual json serialize() const;
            virtual void deserialize(const json& _j);

        private : 
            Date _start; //bp cgs
            Date _end; //bp cgs
                       //
        public : 
#include "./Period_gen.h"
    };

}

namespace std
{
    // to make a period hasable and be in a map
    template<>
        struct hash<ml::Period>
        {
            std::size_t operator()(const ml::Period& period) const noexcept
            {
                // Combine the hash values of _start and _end
                // Assuming Date has a hash specialization, or you implement one
                std::size_t h1 = std::hash<int64_t>()(period.start().time());
                std::size_t h2 = std::hash<int64_t>()(period.end().time());
                return h1 ^ (h2 << 1); // Combine the two hashes
            }
        };
}
