#include "./AsyncFilesystem.h"
namespace ml
{
    std::string FileObject::asString() const
    {
        std::string _r;
        char* data = (char*)_data;
        return std::string(data, _size);
    }

    std::vector<char> FileObject::asVector() const
    {
        return std::vector<char>((char*)_data, (char*)_data + _size);
    }

    AsyncFilesystem::AsyncFilesystem(const std::string& root) : _root(root)
    {

    }
}
