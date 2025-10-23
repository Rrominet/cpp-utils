#include "VertexBufferLayout.h"

VertexBufferLayout::VertexBufferLayout()
{

}

VertexBufferLayout::~VertexBufferLayout()
{
    for (auto & el : elmts)
        delete el;

    elmts.clear();
}


void VertexBufferLayout::push(const unsigned int &pcount, const unsigned int& ptype)
{
    stride += pcount * VertexBufferElmt::sizeOfType(ptype);
    VertexBufferElmt *el = new VertexBufferElmt;
    el->type = ptype;
    el->count = pcount;
    if (ptype == GL_UNSIGNED_BYTE)
        el->normalized = true;
    else
        el->normalized = false;
    elmts.push_back(el);
}
