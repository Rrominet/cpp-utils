#ifndef _GL_INDEXBUFFER_H
#define _GL_INDEXBUFFER_H

class IndexBuffer
{
    private:
        unsigned int glId;
        unsigned int count;
        bool _generated = false;
    public:
        IndexBuffer(); 
        IndexBuffer(const unsigned int* data, unsigned int pcount);
        ~IndexBuffer();

        void generate();
        void bind();
        void unbind();

        void setData(const unsigned int* data, unsigned int pcount);

        unsigned int getCount();
};

#endif
