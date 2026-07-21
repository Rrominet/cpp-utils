#include "./Socket.h"
namespace ml
{
    namespace nodes 
    {
        bool BaseSocket::hasError() const
        {
            return !_typeError.empty();
        }

        std::string BaseSocket::typeError() const
        {
            return _typeError;
        }

        std::string BaseSocket::errors() const
        {
            return _typeError;
        }
    }
}
