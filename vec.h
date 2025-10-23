#pragma once 
#include <random>
#include <typeindex>
#include <vector>
#include <math.h>
#include <string>
#include <algorithm>
#include "debug.h"
#include <functional>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace vc
{
    using namespace std;

    template <typename T>
        int remove(vector<T*>* v, T* elmt, const bool &deleteElmt = false)
        {
            if (v->size() == 0)
                return -1;
            int i = -1; 
            vector<T*>& ref = *v;
            for (unsigned int j=0; j<ref.size(); j++)
            {
                if (ref[j] == elmt)
                {
                    i = static_cast<int>(j);
                    break;
                }
            }

            if (i == -1)
                return -1;

            if(deleteElmt)
                delete ref[static_cast<unsigned int>(i)];

            ref.erase(ref.begin() + i);
            return i;
        }

    template <typename T>
        int remove(vector<T>* v, T elmt, const std::function<bool (const T&, const T&)>& same)
        {
            for (unsigned int i=0; i<v->size(); i++)
            {
                if (same(elmt, v->at(i)))
                {
                    v->erase(v->begin() + i);
                    return i;
                }
            }

            lg("Could not find the element to remove");
            return -1;
        }

    template <typename T>
        int remove(vector<T>* v, const T& elmt)
        {
            for (unsigned int i=0; i<v->size(); i++)
            {
                if (elmt == v->at(i))
                {
                    v->erase(v->begin() + i);
                    return static_cast<int>( i );
                }
            }

            lg("Could not find the element to remove");
            return -1;
        }

    int remove(vector<string>* v, const string& s);

    template <typename T>
        T getMax(vector<T>*v)
        {
            vector<T>& ref = *v;
            T _r = ref[0];
            for (unsigned int i=0; i<ref.size(); i++)
            {
                if (ref[i]>_r)
                    _r = ref[i];
            }
            return _r;
        }

    template <typename T>
        T getMin(vector<T>*v)
        {
            vector<T>& ref = *v;
            T _r = ref[0];
            for (unsigned int i=0; i<ref.size(); i++)
            {
                if (ref[i]<_r)
                    _r = ref[i];
            }
            return _r;
        }

    // carefull : it make a basolute value of the value calculating the average...
    template <typename T>
        double average(const vector<T> &data, const int& start, int end)
        {
            if (end == -1||end >=data.size())
                end = data.size() -1;

            double _r = 0.0;
            double sum = 0.0;

            for (unsigned int i=start; i<end; i++)
                sum += std::abs(data[i]);

            _r = (sum*2)/(end-start + 1);

            return _r; 
        }

    template <typename T>
        bool contains(const vector<T> &data, const T &elmt)
        {
            return std::find(data.begin(), data.end(), elmt) != data.end();
        }

    template <typename T>
        int find(const vector<T> &data, const T &elmt)
        {
            auto it = std::find(data.begin(), data.end(), elmt);
            if (it == data.end())
                return -1;

            return it - data.begin();
        }

    template <typename T>
        bool includes(const vector<T> &data, const T &elmt)
        {
            return vc::contains(data, elmt);
        }

    template <typename T>
        T& last(vector<T> &data)
        {
            return data[data.size() - 1];
        }

    template <typename T>
        T last(const vector<T> &data)
        {
            return data[data.size() - 1];
        }

    template <typename T>
        std::vector<T> reversed(const vector<T> &data)
        {
            std::vector<T> _r;
            for (unsigned int i=data.size() -1; i>0; i--)
                _r.push_back(data[i]);
            return _r;
        }

    template <typename T>
        T shift(vector<T> &data)
        {
            if (data.size() == 0)
                throw std::length_error("vector is empty, can't remove the first elmt.");
            auto _r = data[0];
            data.erase(data.begin());
            return _r;
        }

    template <typename T> 
        void clear(vector<T> &data, const bool& deleteAll=true)
        {
            if (deleteAll)
            {
                for (unsigned int i=0; i<data.size(); i++)
                {
                    if (data[i])
                        delete data[i];
                }
            }
            data.clear();
        }
    template <typename S>
        string join(const vector<string>& data, const S& delimiter)
        {
            string _r = "";
            for (const auto &s : data)
                _r += s + delimiter;
            _r.pop_back();

            return _r;
        }

    template <typename T>
        void prepend(vector<T> &data, T&& elmt)
        {
            data.insert(data.begin(), std::forward<T>(elmt));
        }

    template <typename T>
        void prepend(vector<T> &data, const T& elmt)
        {
            data.insert(data.begin(), std::forward<T>(elmt));
        }

    template <typename T>
        void prepend(vector<T> &data, T& elmt)
        {
            data.insert(data.begin(), elmt);
        }
    //
    //
    template <typename T>
        void move(std::vector<T>& src, std::vector<T>&dest, const T& elmt)
        {
            auto it = std::find(src.begin(), src.end(), elmt);
            if (it != src.end())
            {
                dest.push_back(std::move(*it));
                src.erase(it);
            }
        }

    template <typename T>
        void move_if(std::vector<T>& src, std::vector<T>&dest, const std::function<bool(const T&)>& comp)
        {
            auto it = std::find_if(src.begin(), src.end(), comp);
            if (it != src.end())
            {
                dest.push_back(std::move(*it));
                src.erase(it);
            }
        }
}

namespace ml
{
    template<typename T>
        class Vec
        {
            public : 
                std::vector<T> vec;

                Vec() : vec(){}
                Vec(const std::vector<T>& pvec) : vec(pvec){}
                Vec(std::vector<T>&& pvec) : vec(pvec){}
                Vec(const Vec<T>& other) : vec(other.vec){}
                Vec(std::initializer_list<T> list) : vec(list){}

                operator std::vector<T>& () const {return vec;}
                operator std::vector<T>& () {return vec;}

                typename std::vector<T>::reference at(size_t idx){return vec.at(idx);}
                typename std::vector<T>::reference operator[](size_t idx){return vec[idx];}

                typename std::vector<T>::const_reference at(size_t idx)const {return vec.at(idx);}
                typename std::vector<T>::const_reference operator[](size_t idx)const {return vec[idx];}

                Vec<T>& operator=(const Vec<T>& other){vec = other.vec; return *this;}
                Vec<T>& operator=(Vec<T>&& other){vec = other.vec; return *this;}

                const size_t size()const {return vec.size();}
                const size_t length()const {return vec.size();}

                typename std::vector<T>::reference first() {return vec[0];}
                typename std::vector<T>::const_reference first()const {return vec[0];}

                typename std::vector<T>::reference front() {return vec[0];}
                typename std::vector<T>::const_reference front()const {return vec[0];}

                typename std::vector<T>::reference last() {return vec[this->size() - 1];}
                typename std::vector<T>::const_reference last()const {return vec[this->size() - 1];}

                typename std::vector<T>::reference back() {return vec[this->size() - 1];}
                typename std::vector<T>::const_reference back()const {return vec[this->size() - 1];}

                typename std::vector<T>::iterator begin(){return vec.begin();}
                typename std::vector<T>::const_iterator begin() const{return vec.begin();}

                typename std::vector<T>::iterator end(){return vec.end();}
                typename std::vector<T>::const_iterator end() const{return vec.end();}

                void push_back(const T& v){vec.push_back(v);}
                void push_back(T&& v){vec.push_back(std::move(v));}

                void push(const T& v){vec.push_back(v);}
                void push(T&& v){vec.push_back(std::move(v));}

                void add(const T& v){vec.push_back(v);}
                void add(T&& v){vec.push_back(std::move(v));}

                void append(const T& v){vec.push_back(v);}
                void append(T&& v){vec.push_back(std::move(v));}

                int index(const T& elmt)
                {
                    for (unsigned int i=0; i<vec.size(); i++)
                    {
                        if (vec[i] == elmt)
                            return i;
                    }

                    throw std::runtime_error("The element is not in the vector.");
                }

                void pop_back(){vec.pop_back();}
                T pop()
                {
                    if (vec.size()==0)
                        throw(std::length_error("The size of the vector is 0."));
                    T _r(this->last());
                    vec.pop_back();
                    return _r;
                }

                bool operator==(const Vec<T>& other) const
                {
                    if (vec.size() != other.size())
                        return false;

                    for (unsigned int i=0; i<this->size(); i++)
                    {
                        if (vec[i] != other[i]) 
                            return false;
                    }

                    return true;
                }

                bool operator!=(const Vec<T>& other) const
                {
                    return !(other == *this);
                }

                int remove(const T& elmt){return vc::remove(&vec, elmt);}
                int remove(T& elmt){return vc::remove(&vec, elmt);}
                int removeByValue(const T& elmt){return vc::remove(&vec, elmt);}
                int removeByValue(T& elmt){return vc::remove(&vec, elmt);}
                void remove(int idx){vec.erase(vec.begin() + idx);}
                void removeByIndex(int idx){vec.erase(vec.begin() + idx);}
                // use when T is a pointer !
                int del(T elmt){return vc::remove(&vec, elmt, true);}

                T shift(){return std::move(vc::shift(this->vec));}
                void prepend(const T& v){vc::prepend(vec, v);}
                void prepend(T&& v){vc::prepend(vec, std::move(v));}
                void prepend(T& v){vc::prepend(vec, v);}

                void clear(bool deleteAll=false){vec.clear();}

                bool includes(const T& elmt) const{return vc::includes(vec, elmt);}
                bool contains(const T& elmt) const {return this->includes(elmt);}

                int find(const T& elmt)const{return vc::find(vec, elmt);}

                void iterate(std::function<void (const T& elmt)> f) const
                {
                    for (const auto& e : vec)
                        f(e);
                }

                // if f return false, the elmt is removed from the vector
                void filter(std::function<bool (const T& elmt)> f)
                {
                    std::vector<T> _toRemove;
                    for (const auto& e : vec)
                    {
                        if (f(e) == false)
                            _toRemove.push_back(e);
                    }

                    for (const auto& e : _toRemove)
                        this->remove(e);
                }

                void iterate(std::function<void (T& elmt)> f)
                {
                    for (auto& e : vec)
                        f(e);
                }

                // call all callable object in this vector
                // the callable objects need to be of type void()
                void exec()
                {
                    for (auto& e : vec)
                        e();
                }
                // call all callable object in this vector
                // the callable objects need to be of type void()
                void exec() const
                {
                    for (const auto& e : vec)
                        e();
                }

                void sort()
                {
                    std::sort(vec.begin(), vec.end());
                }

                void sort(std::function<bool (T& a, T& b)> compare)
                {
                    std::sort(vec.begin(), vec.end(), compare);
                }

                Vec<T> sorted()
                {
                    Vec<T> _r(vec);
                    _r.sort();
                    return _r;
                }

                Vec<T> sorted(std::function<bool (T& a, T& b)> compare)
                {
                    Vec<T> _r(vec);
                    _r.sort(compare);
                    return _r;
                }

                void moveToEnd(const T& elmt)
                {
                    auto idx = this->find(elmt);
                    if (idx == -1)
                        return;
                    vec.erase(vec.begin() + idx);
                    vec.push_back(elmt);
                }

                void move(int from, int to)
                {
                    if (from == to)
                        return;
                    if (from < 0 || from >= vec.size())
                        return;
                    if (to < 0 || to >= vec.size())
                        return;

                    if (from < to)
                        std::rotate(vec.rend() - from - 1, vec.rend() - from, vec.rend() - to);
                    else 
                        std::rotate(vec.begin() + from, vec.begin() + from + 1, vec.begin() + to + 1);
                }

                void move(const T& elmt, int movement)
                {
                    if (movement == 0)
                        return;
                    int idx = this->find(elmt);
                    if (idx == -1)
                        return;
                    this->move(idx, idx + movement);
                }

                void setPosition(const T& elmt, unsigned int pos)
                {
                    int idx = this->find(elmt);
                    if (idx == -1)
                        return;
                    this->move(idx, pos);
                }

                json serialize();
                void deserialize(const json& data);

                void concat(const std::vector<T>& other)
                {
                    this->vec.insert(end(), other.begin(), other.end());
                }

                // (1) insert: single value
                typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator position, const T& val) {
                    return vec.insert(position, val);
                }

                // (2) insert: fill
                typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator position, typename std::vector<T>::size_type n, const T& val) {
                    return vec.insert(position, n, val);
                }

                // (3) insert: range (InputIterator first, InputIterator last)
                template <typename InputIterator>
                    typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator position, InputIterator first, InputIterator last) {
                        return vec.insert(position, first, last);
                    }

                // (4) insert: move (rvalue)
                typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator position, T&& val) {
                    return vec.insert(position, std::move(val));
                }

                // (5) insert: initializer list
                typename std::vector<T>::iterator insert(typename std::vector<T>::const_iterator position, std::initializer_list<T> il) {
                    return vec.insert(position, il);
                }

                T join(const T& joinChar) const
                {
                    T _r;
                    for (int i=0; i<vec.size(); i++)
                    {
                        const auto& elmt = vec[i];
                        if (i< vec.size()-1)
                            _r += elmt + joinChar;
                        else 
                            _r += elmt;
                    }
                    return _r;
                }

                // FIXME not sure it works... need to be tested properly.
                void removeDoubles(std::function<bool (T& a, T& b)> f = 0)
                {
                    if (!f)
                        f = [](T& a, T& b) {return a == b;};
                    std::vector<T> garbage = vec;
                    vec.clear();
                    for (auto& elmt : garbage)
                    {
                        if (f(elmt, elmt))
                        {
                            if (!this->includes(elmt))
                                vec.push_back(elmt);
                        }
                    }

                }

                bool empty() const{return vec.empty();}
                void resize(size_t size){vec.resize(size);}
                void resize(size_t size, const T& elmt){vec.resize(size, elmt);}
                void swap(size_t i, size_t j){std::swap(vec[i], vec[j]);}
                void reserve(size_t size){vec.reserve(size);}

                void randomizeOrder()
                {
                    static std::random_device rd;
                    static std::mt19937 rng(rd());
                    std::shuffle(vec.begin(), vec.end(), rng);
                }
        };

    //TODO
    template <typename T>
        json Vec<T>::serialize(){return json {};}

    template <typename T>
        void Vec<T>::deserialize(const json& data){}

}

