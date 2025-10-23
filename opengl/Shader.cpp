#include "Shader.h"

#include <GL/glew.h>

#include <iostream>
#include <fstream>
#include <opengl/errors.h>
#include <debug.h>

Shader::Shader()
{
    glId = 0;
}

Shader::Shader(const std::string& pfilepath)
{
    filepath = pfilepath;
    glId = 0;

    create();
}

Shader::~Shader()
{
    gl_call(glDeleteProgram(glId))    ;
}

void Shader::bind()
{
   gl_call(glUseProgram(glId)) ;
}

void Shader::unbind()
{
   gl_call(glUseProgram(0)) ;
}

void Shader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    bind();
   gl_call(glUniform4f(uniformLocation(name), v0, v1, v2, v3)) ;
}

int Shader::uniformLocation(const std::string& name)
{
   gl_call(int location = glGetUniformLocation(glId, name.c_str()));
   if (location == -1)
       lg2("Uniform in a shader does not exists", name);
   return location;
}

unsigned int Shader::compile(const std::string &src, unsigned int type)
{
    gl_call(unsigned int id = glCreateShader(type));
    const char* srcc = src.c_str();
    gl_call(glShaderSource(id, 1, &srcc, nullptr));
    gl_call(glCompileShader(id));

    int res;
    gl_call(glGetShaderiv(id, GL_COMPILE_STATUS, &res));
    if (!res)
    {
        lg2("Error in compiling the shader", id);
        gl_call(glDeleteShader(id));
        return 0;
    }

    if (type == GL_VERTEX_SHADER)
        vs = id;
    else if (type == GL_FRAGMENT_SHADER)
        fs = id;

    return id;
}

unsigned int Shader::create()
{
   this->parse();
   glId = glCreateProgram();
   compile(vsSrc, GL_VERTEX_SHADER);
   compile(fsSrc, GL_FRAGMENT_SHADER);

   gl_call(glAttachShader(glId, vs));
   gl_call(glAttachShader(glId, fs));
   gl_call(glLinkProgram(glId));
   gl_call(glValidateProgram(glId));

   gl_call(glDeleteShader(vs));
   gl_call(glDeleteShader(fs));

   return glId;
}

unsigned int Shader::create(const std::string &vShader, const std::string &fgShader)
{
   glId = glCreateProgram();
   compile(vShader, GL_VERTEX_SHADER);
   compile(fgShader, GL_FRAGMENT_SHADER);

   gl_call(glAttachShader(glId, vs));
   gl_call(glAttachShader(glId, fs));
   gl_call(glLinkProgram(glId));
   gl_call(glValidateProgram(glId));

   gl_call(glDeleteShader(vs));
   gl_call(glDeleteShader(fs));

   return glId;
}

void Shader::parse()
{
    std::ifstream stream(filepath);
    std::string line;
    vsSrc = "";
    fsSrc = "";

    bool frag = false;

    while(getline(stream, line))
    {
        if (line.find("/*FRAGMENT*/") != std::string::npos)
        {
            frag = true;
            continue;
        }

        if (!frag)
            vsSrc += line + "\n";
        else
            fsSrc += line + "\n";
    }

}

void Shader::setUniform1i(const std::string &name, const int &v)
{
    bind();
    gl_call(glUniform1i(uniformLocation(name), v));
}

void Shader::setUniform1f(const std::string &name, const float &v)
{
    bind();
    gl_call(glUniform1f(uniformLocation(name), v));
}

void Shader::setUniformMatrix4fv(const std::string& name, glm::mat4 matrix)
{
    bind();
    gl_call(
        glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix))
    );
}

