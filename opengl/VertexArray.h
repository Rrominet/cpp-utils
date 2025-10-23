#pragma once

class VertexBufferLayout;
class VertexBuffer;
class VertexArray
{
    private :
        unsigned int glId;
        bool _generated = false;
    public :
        VertexArray();
        ~VertexArray();

        void generate();
        void addBuffer(VertexBuffer *vb, VertexBufferLayout *layout);

        void bind();
        void unbind();
};
