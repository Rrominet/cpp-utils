#include "VertexBuffer.h"
#include "errors.h"

VertexBuffer::VertexBuffer()
{

}

VertexBuffer::VertexBuffer(const void* data, unsigned int size)
{
    setData(data, size);
}

VertexBuffer::~VertexBuffer()
{
    gl_call(glDeleteBuffers(1, &glId));
}

void VertexBuffer::bind()
{
    gl_call(glBindBuffer(GL_ARRAY_BUFFER, glId));
}

void VertexBuffer::unbind()
{
    gl_call(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer::setData(const void* data, unsigned int size)
{
    if (!_generated)
        generate();
    gl_call(glBindBuffer(GL_ARRAY_BUFFER, glId));
    gl_call(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

void VertexBuffer::generate()
{
    gl_call(glGenBuffers(1, &glId));
    _generated = true;
}
