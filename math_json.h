#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "./mlMath.h"

namespace math
{
    json serialize(const math::vec2d& v);
    json serialize(const math::vec3d& v);
    json serialize(const math::vec4d& v);
    json serialize(const math::matrix3x3& m);
    json serialize(const math::matrix4x4& m);

    void deserialize(math::vec2d& v    , const json& data);
    void deserialize(math::vec3d& v    , const json& data);
    void deserialize(math::vec4d& v    , const json& data);
    void deserialize(math::matrix3x3& m, const json& data);
    void deserialize(math::matrix4x4& m, const json& data);
}
