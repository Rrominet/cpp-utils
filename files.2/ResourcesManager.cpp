#include "./ResourcesManager.h"
#include <iostream>
#include <fstream>

namespace ml
{
    void Resource::load(const std::string& filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error("Unable to open file " + filepath);

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg); // Go back to the beginning of the file

        _data.resize(fileSize);
        if (!file.read(_data.data(), fileSize))
            throw std::runtime_error("Error reading file " + filepath);

        _loaded = true;
        _events.emit("loaded", this);
    }

    const std::vector<char>& Resource::data() const
    {
        if (!_loaded)
            throw std::runtime_error("resource::data(), Resource not loaded yet.");
        return _data;
    }

    std::vector<char>& Resource::data()
    {
        if (!_loaded)
            throw std::runtime_error("resource::data(), Resource not loaded yet.");
        return _data;
    }
}
