#pragma once
#include <string>
#include <vector>

namespace ml
{
    namespace ai
    {
        std::string filesForLLM(const std::vector<std::string>& files, const std::string& root="");
        std::string fileForLLM(const std::string& filepath, const std::string& root="");
    }
}
