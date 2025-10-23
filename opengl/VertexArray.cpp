#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "errors.h"
#include "debug.h"

VertexArray::VertexArray()
{
    lg2("id", glId); 
}

VertexArray::~VertexArray()
{
    gl_call(glDeleteVertexArrays(1, &glId))    ;
}

void VertexArray::generate()
{
   gl_call(glGenVertexArrays(1, &glId)) ;
   lg("Vertex Array generated");
   lg2("id", glId);
   _generated = true;
}

void VertexArray::addBuffer(VertexBuffer *vb, VertexBufferLayout *layout)
{
    lg2("generated", _generated);
    if (!_generated)
        generate();
    bind();
    vb->bind();
    unsigned int offset = 0;
    const auto& elmts = layout->getElmts();
    for (unsigned int i = 0; i< elmts.size(); i++)
    {
        const auto& elmt = elmts[i];
        gl_call(
            glVertexAttribPointer(i, elmt->count, elmt->type, elmt->normalized, layout->getStride(), (const void*)offset)
            );
            std::cout << "glVertexAttribPointer(" << i << ", " << elmt->count << ", " << elmt->normalized << ", " << layout->getStride() << ", " << offset << ")" << std::endl;
        offset += elmt->count * VertexBufferElmt::sizeOfType(elmt->type);
        gl_call(glEnableVertexAttribArray(i));
    }
}

void VertexArray::bind()
{
    lg2("id", glId); 
    gl_call(glBindVertexArray(glId)) ;
}

void VertexArray::unbind()
{
    gl_call(glBindVertexArray(0)) ;
}

