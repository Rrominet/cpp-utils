#pragma once

#include <vector>
#include <GL/glew.h>

struct VertexBufferElmt
{
    unsigned int type;
    unsigned int count;
    bool normalized;

    static unsigned int sizeOfType(unsigned int type)
    {
        switch(type)
        {
            case GL_FLOAT: return 4 ;
            case GL_UNSIGNED_INT : return 4;
            case GL_UNSIGNED_BYTE : return 1;
        }
        return 0;
    }
};

class VertexBufferLayout
{
    private :
        std::vector<VertexBufferElmt*> elmts;
        unsigned int stride = 0;

    public :
        VertexBufferLayout();
        ~VertexBufferLayout();
        void push(const unsigned int &pcount, const unsigned int& ptype);

        inline const std::vector<VertexBufferElmt*> getElmts() const {return elmts;}
        inline unsigned int getStride() const {return stride;}

};
