#include "errors.h"
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Renderer.h"
#include "Image2D.h"
#include "debug.h"


Renderer::Renderer(const float &x , const float &y )
{
    setDimensions(x, y);
}

void Renderer::setDimensions(const float &x, const float &y)
{
    _x = x;
    _y = y;
    _ratio = _y/x;

    proj = glm::ortho(-_x/2, _x/2, -_y/2, _y/2, -1.0f, 1.0f);
}

void Renderer::clear()
{
    gl_call(glClear(GL_COLOR_BUFFER_BIT));
}

void Renderer::draw(VertexArray* va, IndexBuffer* ib, Shader* shader)
{
    shader->setUniformMatrix4fv("proj", proj);
    shader->bind();
    va->bind();
    ib->bind();
    gl_call(glDrawElements(renderMode, ib->getCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::clearNDraw(VertexArray* va, IndexBuffer* ib, Shader* shader)
{
    clear();
    draw(va, ib, shader);
}

void Renderer::clearNDraw(Image2D* img)
{
    if(!img->texture()->data())
    {
        lg("data of the image is not loaded. ");
        return;
    }
    img->texture()->bind();
    clearNDraw(img->va(), img->ib(), img->shader());
}

