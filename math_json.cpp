#include "./math_json.h"

namespace math
{

    json serialize(const math::vec2d& v)
    {
        json _r = json::array();
        _r.push_back(v.x());
        _r.push_back(v.y());
        return _r;
    }
    json serialize(const math::vec3d& v)
    {
        json _r = json::array();
        _r.push_back(v.x());
        _r.push_back(v.y());
        _r.push_back(v.z());
        return _r;
    }
    json serialize(const math::vec4d& v)
    {
        json _r = json::array();
        _r.push_back(v.x());
        _r.push_back(v.y());
        _r.push_back(v.z());
        _r.push_back(v.w());
        return _r;
    }
    json serialize(const math::matrix3x3& m)
    {
        json _r = json::array();
        for (int i=0; i<3; i++)
        {
            for (int j=0; j<3; j++)
                _r.push_back(m(i, j));
        }

        return _r;
    }

    json serialize(const math::matrix4x4& m)
    {
        json _r = json::array();
        for (int i=0; i<4; i++)
        {
            for (int j=0; j<4; j++)
                _r.push_back(m(i, j));
        }

        return _r;
    }

    void deserialize(math::vec2d& v    , const json& data)
    {
        v = math::vec2d(data[0], data[1]);
    }

    void deserialize(math::vec3d& v    , const json& data)
    {
        v = math::vec3d(data[0], data[1], data[2]);
    }

    void deserialize(math::vec4d& v    , const json& data)
    {
        v = math::vec4d(data[0], data[1], data[2], data[3]);
    }

    void deserialize(math::matrix3x3& m, const json& data)
    {
        int idx = 0;
        for (int i = 0; i<3; i++)
        {
            for (int j = 0; j<3; j++)
            {
                if (idx > data.size())
                    throw "The json size for the 3x3 matrix is too small : " + std::to_string(data.size());
                m(i, j) = data[idx] ;
                idx++;
            }
        }
    }

    void deserialize(math::matrix4x4& m, const json& data)
    {
        int idx = 0;
        for (int i = 0; i<4; i++)
        {
            for (int j = 0; j<4; j++)
            {
                if (idx > data.size())
                    throw "The json size for the 4x4 matrix is too small : " + std::to_string(data.size());
                m(i, j) = data[idx] ;
                idx++;
            }
        }
    }
}
