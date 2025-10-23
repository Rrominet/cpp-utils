#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
    private :
        std::string filepath;
        unsigned int glId;
        int uniformLocation(const std::string& name);

    public :
        Shader(); 
        Shader(const std::string& pfilepath);
        ~Shader();

        std::string vsSrc;
        std::string fsSrc;

        unsigned int vs;
        unsigned int fs;
        unsigned int prg;

        void bind();
        void unbind();

        unsigned int compile(const std::string &src, unsigned int type);
        unsigned int create();
        unsigned int create(const std::string &vShader, const std::string &fgShader);

        void parse();

        //set uniforms //
        void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
        void setUniform1i(const std::string &name, const int &v);
        void setUniform1f(const std::string &name, const float &v);
        void setUniformMatrix4fv(const std::string &name, glm::mat4 matrix);
};

