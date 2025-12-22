#pragma once
#include <vector>
#include <math.h>
#include <cstdlib>
#include <string>
#include "vec.h"
#include "debug.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// default is 16 bits precision
template<typename T=float>
class Color
{
    private : 
        T _r = 0;
        T _g = 0;
        T _b = 0;
        T _a = 0;

        // difference between max in rgb and min in rgb
        T _chroma = 0;

        T _h;
        T _s;
        T _v;

    public : 
        T r()const {return _r;}
        T g()const {return _g;}
        T b()const {return _b;}
        T a()const {return _a;}

        T x()const {return _r;}
        T y()const {return _g;}
        T z()const {return _b;}

        T red()const {return _r;}
        T green()const {return _g;}
        T blue()const {return _b;}
        T alpha()const {return _a;}

        T chroma()const {return _chroma;}

        double hue()const {return _h;}
        double saturation()const {return _s;}
        double value()const {return _v;}

        double h()const {return _h;}
        double s()const {return _s;}
        double v()const {return _v;}

        Color(const T &r, const T &g, const T &b, const T& a=1){set(r, g, b, a);}
        Color(const T &val, const T& a=1){set(val, val, val, a);}
        ~Color(){}

        void set(const T &r, const T &g, const T &b, const T& a=1)
        {
            _r = r; _g = g; _b = b; _a = a;
            updateFromRGB();
        }
        void setRGB(const T &r, const T &g, const T &b){set(r, g, b, 1);}
        void setRGBA(const T &r, const T &g, const T &b, const T& a){set(r, g, b, a);}
        void setHSV(const T &h, const T &s, const T &v)
        {
            _h = h; _s = s; _v = v;
            updateFromHSV();
        }

        void updateFromRGB()
        {
            std::vector<T> v {_r, _g, _b};
            auto max = vc::getMax(&v);
            auto min = vc::getMin(&v);
            _chroma = max - min;

            if (max == min)
                _h = 0.0;
            else if (max == _r)
                _h = fmod((60.0 * ((_g-_b)/_chroma) + 360.0), 360.0);
            else if (max == _g)
                _h = fmod((60.0 * ((_b-_r)/_chroma) + 120.0), 360.0);
            else if (max == _b)
                _h = fmod((60.0 * ((_r-_g)/_chroma) + 240.0), 360.0);

            if (max == 0)
                _s = 0.0;
            else 
                _s = _chroma/max;
            _v = max;
        }

        void updateFromHSV()
        {
            if (_s == 0)
            {
                _r = _v; _b = _v; _g = _v;
                return;
            }

            _chroma = _s * _v;
            auto max = _v;
            auto min = max - _chroma;

            auto x = _chroma * (1 - abs(fmod(_h/60.0, 2) -1));
            if (0.0<=_h && _h<60.0)
            {
                _r = _chroma;
                _g = x;
                _b = 0;
            }

            else if (60.0<=_h && _h<120.0)
            {
                _r = _chroma;
                _g = x;
                _b = 0;
            }

            else if (120.0<=_h && _h<180.0)
            {
                _r = 0;
                _g = _chroma;
                _b = x;
            }

            else if (180.0<=_h && _h<240.0)
            {
                _r = 0;
                _g = x;
                _b = _chroma;
            }

            else if (240.0<=_h && _h<300.0)
            {
                _r = x;
                _g = 0;
                _b = _chroma;
            }

            else if (300.0<=_h && _h<360.0)
            {
                _r = _chroma;
                _g = 0;
                _b = x;
            }

            _r += min; _g += min; _b += min;
        }

        static Color fromHSV(const double &h, const double &s, const double &v)
        {
            Color c;
            c.setHSV(h, s , v);
            return c;
        }

        json serialize() const
        {
            json data;
            data["r"] = _r;
            data["g"] = _g;
            data["b"] = _b;
            data["a"] = 1.0; // temp

            return data;
        }

        void deserialize(const json &data)
        {
            _r = data["r"];
            _g = data["g"];
            _b = data["b"];
            _a = data["a"];

            updateFromRGB();
        }

        std::string stringify()
        {
            return serialize().dump();
        }
};
