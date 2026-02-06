#pragma once

#include <cmath>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace geometry
{
    struct Size
    {
        int w; 
        int h;

        int width(){return w;}
        int height(){return h;}

        Size(){}
        ~Size(){}
        Size(int x, int y=-1);
        json serialize();
        void deserialize(const json &data);
    };

    template <typename T=float>
    struct Point
    {
        T x;
        T y;
        Point() : x(0), y(0){}
        Point(T x, T y) : x(x), y(y){}
    };

    template <typename T>
        double squareDistance(const T &xa, const T &ya, 
                const T &xb, const T &yb)
        {
            return ((xb - xa)*(xb - xa)) + ((yb - ya) * (yb - ya));
        }
    template <typename T>
        T distance(const T &xa, const T &ya, 
                const T &xb, const T &yb)
        {
            return sqrt(geometry::squareDistance(xa, ya, xb, yb));
        }
}
