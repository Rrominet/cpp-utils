#pragma once 
#include <string>
#include <iostream> 
#include <iomanip>
#include "str.h"
#include <Eigen/Dense>

namespace math
{
    // need to add template
    template<typename T = double>
        std::string round(T v, const int &round=2)
        {
            std::string s = std::to_string(v);
            s = str::replace(s, ",", ".");
            std::vector<std::string> tmp = str::split(s, ".");
            std::string i = tmp[0];
            if (tmp.size()==1)
                return i;
            std::string fl = tmp[1];
            fl = fl.substr(0, round);

            return i + "." + fl;
        }

    template<typename T = float>
        T lerp(const T & a, const T & b, const T & ratio)
        {
            return (a*(1.0 - ratio)) + (b * ratio);
        }

    //return the ratio between 0 and 1
    template<typename T = float>
        T inverseLerp(const T & a, const T & b, const T & lerp)
        {
            T interval = b - a;
            T lerpInterval = lerp - a;
            return lerpInterval / interval;
        }

    template<typename T=float>
        T clamp(T nb, const float& max = 1.0, const float& min=0.0)
        {
            if (nb>max)
                nb = max;
            if (nb<min)
                nb = min;
            return nb;
        }

    template<typename T=float>
        void clamp(T* nb, const float& min = 0.0, const float& max=1.0)
        {
            if (*nb>max)
                *nb = max;
            if (*nb<min)
                *nb = min;
        }

    template<typename T=float>
        float average(const std::vector<T>& table, bool absolute=false, int begin=0, int end=-1)
        {
            T _r = 0;
            if (end == -1)
                end = table.size();
            for (int i=begin; i<end; i++)
            {
                if (absolute)
                    _r += abs(table[i]);
                else 
                    _r += table[i];
            }

            return (float)(_r)/(float)table.size();
        }
    template<typename T=float>
        float max(const std::vector<T>& table, bool absolute=false, int begin=0, int end=-1)
        {
            T _r = 0;
            if (end == -1)
                end = table.size();
            for (int i=begin; i<end; i++)
            {
                if (absolute)
                {
                    if (abs(table[i])>_r)
                        _r = abs(table[i]);
                }
                else 
                {
                    if (table[i]>_r)
                        _r = table[i];
                }
            }

            return _r;
        }

    template<typename T=float>
        float min(const std::vector<T>& table, bool absolute=false, int begin=0, int end=-1)
        {
            T _r = 0;
            if (end == -1)
                end = table.size();
            for (int i=begin; i<end; i++)
            {
                if (absolute)
                {
                    if (abs(table[i])<_r)
                        _r = abs(table[i]);
                }
                else 
                {
                    if (table[i]<_r)
                        _r = table[i];
                }
            }

            return _r;
        }

    template<typename T=float>
        T min(T n1, T n2)
        {
            if (n1<=n2)
                return n1;
            return n2;
        }

    template<typename T=float>
        T max(T n1, T n2)
        {
            if (n1>=n2)
                return n1;
            return n2;
        }
    using vec2d = Eigen::Vector2d;
    using vec3d = Eigen::Vector3d;
    using vec4d = Eigen::Vector4d;
    using color = Eigen::Vector4d;
    using matrix3x3 = Eigen::Matrix3d;
    using matrix4x4 = Eigen::Matrix4d;
}
