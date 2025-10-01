#ifndef _GL_VERTEX_BUFFER_H
#define _GL_VERTEX_BUFFER_H


class VertexBuffer
{
    private:
        unsigned int glId;
        bool _generated = false;

    public:
        VertexBuffer();
        VertexBuffer(const void* data, unsigned int size);
        ~VertexBuffer();

        void generate();
        void setData(const void* data, unsigned int size);
        void bind();
        void unbind();
};

#endif
