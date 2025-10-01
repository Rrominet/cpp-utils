#pragma once
#include <string>

template<typename T>
class Dict
{
    private : 
        std::string _key;
        T _value;

    public : 
        Dict(){}
        Dict(const std::string &key, const T &value)
        {
            _key = key;
            _value = value;
        }
        ~Dict(){}

        std::string key(){return _key;}
        T value() {return _value;}
        T val() {return _value;}

        void setKey(const std::string &key){_key = key;}
        void setValue(const T &value){_value= value;}
        void setVal(const T &value){setValue(value);}

        Dict& operator++()
        {
            _value ++;
            return *this;
        }

        Dict& operator--()
        {
            _value --;
            return *this;
        }

        Dict& operator +=(const T &value)
        {
            _value += value;
            return *this;
        }

        Dict& operator -=(const T &value)
        {
            _value -= value;
            return *this;
        }
};

template<typename T>
Dict<T> operator+( const Dict<T> &d1, const Dict<T> &d2)
{
    Dict<T> d (d1.key(), d1.value());
    d += d2;
    return d;
}

template<typename T>
Dict<T> operator-( const Dict<T> &d1, const Dict<T> &d2)
{
    Dict<T> d (d1.key(), d1.value());
    d -= d2;
    return d;
}
