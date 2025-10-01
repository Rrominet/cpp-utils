#include "IndexBuffer.h"
#include "errors.h"

IndexBuffer::IndexBuffer()
{

}

IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int pcount)
{
    setData(data, pcount);
}

IndexBuffer::~IndexBuffer()
{
    gl_call(glDeleteBuffers(1, &glId));
}

void IndexBuffer::bind()
{
    gl_call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glId));
}

void IndexBuffer::unbind()
{
    gl_call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

unsigned int IndexBuffer::getCount()
{
    return count;
}

void IndexBuffer::setData(const unsigned int* data, unsigned int pcount)
{
    if (!_generated)
        generate();

    count = pcount;

    gl_call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glId));
    gl_call(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

void IndexBuffer::generate()
{
    gl_call(glGenBuffers(1, &glId));
    _generated = true;
}
