#include "json.h"
namespace ml::json
{
    nlohmann::json updated(const nlohmann::json& old, const nlohmann::json& _new)
    {
        nlohmann::json _r;
        for (auto& i : old.items())
            _r[i.key()] = i.value();

        for (auto& i : _new.items())
        {
            if (_r.contains(i.key()))
                _r[i.key()] = i.value();
            else if (_r[i.key()] != i.value())
                _r[i.key()] = i.value();
        }

        return _r;
    }

    void merge(nlohmann::json& src, const nlohmann::json& with)
    {
        ml::Vec<std::string> missing;
        for (auto& b_i : with.items())
        {
            if (!src.contains(b_i.key()))
                missing.push_back(b_i.key());
            for (auto& a_i : src.items())
            {

                if (a_i.key() == b_i.key())
                {
                    if (a_i.value() == b_i.value())
                        continue;

                    else if (a_i.value().is_array())
                    {
                        if (b_i.value().is_array())
                        {
                            for (auto& v : b_i.value())
                                src[a_i.key()].push_back(v);
                        }
                        else 
                            src[a_i.key()].push_back(b_i.value());
                    }

                    else if (a_i.value().is_object())
                    {
                        if (b_i.value().is_object())
                            src[a_i.key()] = merged(a_i.value(), b_i.value());
                    }

                    else 
                    {
                        auto tmp = a_i.value();
                        src[a_i.key()] = nlohmann::json::array();
                        src[a_i.key()].push_back(tmp);
                        src[a_i.key()].push_back(b_i.value());
                    }
                }
            }
        }

        for (auto& m : missing)
            src[m] = with[m];
    }

    nlohmann::json merged(const nlohmann::json& a,const nlohmann::json& b)
    {
        nlohmann::json _r = a;
        if (_r.is_null())
            _r = nlohmann::json::object();

        json::merge(_r, b);
        return _r;
    }


    //return true if a and b are equal (meaning their key and values match)
    bool compare(const nlohmann::json& a, const nlohmann::json& b)
    {
        if (a.type() != b.type())
            return false;
        
        if (a.is_object())
        {
            if (a.size() != b.size())
                return false;
            
            for (auto& a_item : a.items())
            {
                if (!b.contains(a_item.key()))
                    return false;
                
                if (!compare(a_item.value(), b[a_item.key()]))
                    return false;
            }
            return true;
        }
        else if (a.is_array())
        {
            if (a.size() != b.size())
                return false;
            
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (!compare(a[i], b[i]))
                    return false;
            }
            return true;
        }
        else
        {
            return a == b;
        }
    }
}

