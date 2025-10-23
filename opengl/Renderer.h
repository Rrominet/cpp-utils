#ifndef _GL_RENDERER_H
#define _GL_RENDERER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class VertexArray;
class IndexBuffer;
class Shader;
class Image2D;

struct Dimension
{
    float x = 0;
    float y = 0;
};

class Renderer
{
    private :
        unsigned int renderMode = GL_TRIANGLES;
        float _x;
        float _y;
        float _ratio, _xmin, _xmax, _ymin, _ymax;
        glm::mat4 proj;

    public :
        // xx et y are the pixels af the screen;
        Renderer(const float &x = 2.0f, const float &y = 2.0f);
        void setDimensions(const float &x, const float &y);
        void clear();
        void draw(VertexArray* va, IndexBuffer* ib, Shader* shader);
        void clearNDraw(VertexArray* va, IndexBuffer* ib, Shader* shader);
        void clearNDraw(Image2D* img);

        inline Dimension dimensions(){
            Dimension dim;
            dim.x = _x;
            dim.y = _y;
            return dim;
        }

};

#endif
