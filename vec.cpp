#include "vec.h"

namespace vc 
{
    int remove(vector<string>* v, const string& s)
    {
        int _indx = -1; 
        vector<string>& ref = *v;
        for (int i=0; i<ref.size(); i++)
            if (ref[i] == s)
            {
                _indx = i;
                break;
            }

        if (_indx == -1)
            return _indx;
        ref.erase(ref.begin() + _indx);
        return _indx;
    }
}
namespace ml
{

    template<>
        json Vec<std::string>::serialize()
        {
            json data;
            for (auto v : vec)
                data.push_back(v);
            return data;
        }

    template<>
        json Vec<int>::serialize()
        {
            json data;
            for (auto v : vec)
                data.push_back(v);
            return data;
        }

    template<>
        json Vec<double>::serialize()
        {
            json data;
            for (auto v : vec)
                data.push_back(v);
            return data;
        }

    template<>
        json Vec<float>::serialize()
        {
            json data;
            for (auto v : vec)
                data.push_back(v);
            return data;
        }

    template<>
        json Vec<bool>::serialize()
        {
            json data;
            for (auto v : vec)
            {
                if (v)
                    data.push_back(1);
                else 
                    data.push_back(0);
            }
            return data;
        }

    template<>
        void Vec<std::string>::deserialize(const json& data)
        {
            vec.clear();
            for (auto jv : data)
                vec.push_back(jv);
        }
    template<>
        void Vec<float>::deserialize(const json& data)
        {
            vec.clear();
            for (auto jv : data)
                vec.push_back(jv);
        }
    template<>
        void Vec<double>::deserialize(const json& data)
        {
            vec.clear();
            for (auto jv : data)
                vec.push_back(jv);
        }
    template<>
        void Vec<int>::deserialize(const json& data)
        {
            vec.clear();
            for (auto jv : data)
                vec.push_back(jv);
        }
    template<>
        void Vec<bool>::deserialize(const json& data)
        {
            vec.clear();
            for (auto jv : data)
            {
                if (jv)
                    vec.push_back(true);
                else 
                    vec.push_back(false);
            }
        }
}
