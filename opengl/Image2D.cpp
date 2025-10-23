#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Image2D.h"
#include "VertexBufferLayout.h"
#include "debug.h"

#include <FreeImage.h>
#include <FreeImagePlus.h>
#include "img_utils.h"

Image2D::Image2D(const std::string &path)
{
    _img = new fipImage;
    if (!path.empty())
        load(path);
}

Image2D::Image2D(unsigned char* data, const int &w, const int &h, const int &bpc )
{
    _img = new fipImage;
    if (data)
        load(data, w, h, bpc);
}

Image2D::~Image2D()
{
    delete _img;
}

void Image2D::translate(const float &x, const float &y)
{
    transforms = glm::translate(transforms, glm::vec3(x, y, 0.0f));
    _shader.setUniformMatrix4fv("transforms", transforms);
    _x += x;
    _y += y;
}

void Image2D::rotate(const float &angle, const std::vector<float> axe)
{
    if (axe.size() <3)
    {
        lg("You must provide at list a vector with 3 number because it's a 3D vector.");
        return;
    }
    transforms = glm::rotate(transforms, angle*toRadians, glm::vec3(axe[0], axe[1], axe[2]));
    _shader.setUniformMatrix4fv("transforms", transforms);
}

void Image2D::scale(const float &x, const float &y, const float &xPiv, const float &yPiv)
{
    _zoom *=x;
    transforms = glm::translate(transforms, glm::vec3(xPiv - _x, yPiv - _y, 0.0f));
    transforms = glm::scale(transforms, glm::vec3(x ,y, 1.0f));
    transforms = glm::translate(transforms, glm::vec3(-xPiv + _x, -yPiv + _y, 0.0f));
    _shader.setUniformMatrix4fv("transforms", transforms);

    _xpiv = xPiv; 
    _ypiv = yPiv;
}

void Image2D::bgrToRgb()
{
    img::bgrToRgb(_img);
}

void Image2D::rgbToBgr()
{
    this->bgrToRgb(); // CHelou ? 
}

// obsolete, could be replaces with the img::openGLFormatted func but it does not keep in memory the fip object. 
// So for convenience, we kept it.
void Image2D::load(const std::string &path)
{
    lg("loading with lib FreeImagePlus...");
    bool res = _img->load(path.c_str());
    if (res)
        lg("image loaded in CPU");
    else
    {
        lg("Can't load image. Abort.");
        return;
    }

    if (!_img->accessPixels())
    {
        lg("image has no data.");
        return;
    }

    this->bgrToRgb();

    load(_img->accessPixels(), _img->getWidth(), _img->getHeight(), _img->getBitsPerPixel());
    lg("image copied to GPU");
}

void Image2D::load(unsigned char* data, const int &w, const int &h, const int &bpc )
{
    lg2("bpc", bpc);

    _texture.load(data, w, h, bpc);
    lg("texture loaded.");
    float ratio = (h*1.0)/(w*1.0);
    if (ratio<=1)
    {
        float vertices[16] =
        {
            -1.0f, -ratio, 0.0f, 0.0f,
            1.0f, -ratio, 1.0f, 0.0f,
            1.0f,  ratio, 1.0f, 1.0f,
            -1.0f,  ratio, 0.0f, 1.0f,
        };
        lg("vertices load in RAM");
        _vb.setData(vertices, sizeof(vertices));
    }
    else
    {
        float vertices[16] =
        {
            -(1/ratio), -1.0f, 0.0f, 0.0f,
            (1/ratio), -1.0f, 1.0f, 0.0f,
            (1/ratio),  1.0, 1.0f, 1.0f,
            -(1/ratio),  1.0, 0.0f, 1.0f,
        };
        lg("vertices load in RAM");
        _vb.setData(vertices, sizeof(vertices));
    }

    lg("vertices set to the vertex buffer");
    VertexBufferLayout lay;

    lay.push(2, GL_FLOAT);
    lay.push(2, GL_FLOAT);
    lg("Layout setted");
    _va.addBuffer(&_vb, &lay);
    lg("buffer added to the vertex Array");

    unsigned int indices[6] =
    {
        0, 1, 2,
        2, 3, 0
    };

    _ib.setData(indices, 6);
    generateShader();
}

bool Image2D::save(const std::string &path, const int &flag)
{
   this->bgrToRgb();  
   return _img->save(path.c_str(), flag);
}

void Image2D::generateShader()
{
    std::string vs =
        "#version 330 core\n\n"
        "layout(location = 0) in vec4 pos;\n"
        "layout(location = 1) in vec2 tex;\n"
        "out vec2 texCoord;\n"
        "uniform mat4 transforms;\n"
        "uniform mat4 proj;\n"
        "void main(){\n"
        "gl_Position = proj * transforms * pos;\n"
        "texCoord = tex;\n"
        "}";

    std::string fs =
        "#version 330 core\n\n"
        "in vec2 texCoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D textureSampler;\n"
        "void main(){\n"
        "color = texture(textureSampler, texCoord);\n"
        "}";

    _shader.create(vs, fs);
    _shader.setUniformMatrix4fv("transforms", transforms);
}

void Image2D::reinitialiseTransforms()
{
    transforms = glm::mat4(1.0f);
    transforms = glm::scale(transforms, glm::vec3(500.0f, 500.0f, 1.0f));
    _shader.setUniformMatrix4fv("transforms", transforms);

    _zoom = 500; 
    _x = 0; 
    _y = 0;
    _xpiv = 0;
    _ypiv = 0;
}



