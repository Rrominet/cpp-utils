#pragma once
//small utilies function to get infod about the current rogram build information with fxmake
//like versions and dependencies

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace fxmake
{
    std::string version();
    json dependencies();
}
