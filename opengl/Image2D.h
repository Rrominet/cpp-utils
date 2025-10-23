#pragma once

#include "Texture.h"
#include "Shader.h"
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "IndexBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

class fipImage;
class Image2D
{

    private :
        Texture _texture;
        Shader _shader;
        VertexBuffer _vb;
        VertexArray _va;
        IndexBuffer _ib;
        fipImage *_img = nullptr;
        const float toRadians = 3.14159265f / 180.0f;
        glm::mat4 transforms = glm::mat4(1.0f);
        float _zoom = 1.0f;
        float _x = 0.0f, _y = 0.0f, 
        _xpiv = 0.0f, _ypiv = 0.0f;


    public :
        Image2D(const std::string &path = "");
        Image2D(unsigned char* data, const int &w, const int &h, const int &bpc = 8);
        ~Image2D();

        void translate(const float &x, const float &y);
        void rotate(const float &angle, std::vector<float> axe = {0.0f, 0.0f, 1.0f});
        void scale(const float &x, const float &y, const float &xPiv=0, const float &yPiv=0);

        void load(const std::string &path);
        void load(unsigned char* data, const int &w, const int &h, const int &bpc = 8);

        bool save(const std::string &path, const int &flag=0);

        void bgrToRgb();
        void rgbToBgr();

        void generateShader();
        void reinitialiseTransforms(); 

        Texture* texture(){return &_texture;}
        Texture* tex(){return &_texture;}
        Shader* shader(){return &_shader;}
        VertexBuffer* vb(){return &_vb;}
        VertexArray* va(){return &_va;}
        IndexBuffer* ib(){return &_ib;}
        float zoom(){return _zoom;}
        float x(){return _x;}
        float y(){return _y;}
        fipImage* cpuImg(){return _img;}
};
