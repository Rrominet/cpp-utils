#include "Texture.h"
#include "errors.h"
#include "debug.h"
#include <mutex>
#include <string.h>

#include "img_utils.h"
#include "opengl/Texture.h"
#include <thread>

namespace textures_memory
{
    std::atomic_int _count = 0;
    std::atomic_int _cpus = 0;
    std::atomic_int _gpus = 0;
    int countAlive()
    {
        return _count;
    }

    int cpuAlive() {return _cpus;}
    int gpuAlive() {return _gpus;}
}

Texture::Texture ()
{
    _data = nullptr;
#ifdef mydebug
    textures_memory::_count ++;
#endif

}

Texture::Texture (unsigned char* data, const unsigned int &w, const unsigned int &h, const unsigned int &bpp)
{
    _data = nullptr;
    load(data, w, h, bpp);

#ifdef mydebug
    textures_memory::_count ++;
#endif
}

Texture::Texture(const char* filepath, bool sendToGPU)
{
    _path = std::string(filepath);
    this->loadCPU(filepath);
    if (sendToGPU)
        this->sendToGPU();

#ifdef mydebug
    textures_memory::_count ++;
#endif
}

Texture::Texture (fipImage* img)
{
    this->loadCPU(img);
    this->sendToGPU();

#ifdef mydebug
    textures_memory::_count ++;
#endif
}

Texture::~Texture()
{
    this->unload();

#ifdef mydebug
    textures_memory::_count --;
#endif
    lg("~Texture()");
}

void Texture::glInit()
{
    gl_call(glGenTextures(1, &glId));
    lg2("GLid", glId);
    this->bind();

    gl_call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    gl_call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl_call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    this->unbind();
}

void Texture::load(unsigned char* data, const unsigned int &w, const unsigned int &h, const unsigned int &bpp)
{
    this->loadCPU(data, w, h, bpp);
    // this part NEED to be executed on the MAIN THREAD ! :/
    this->sendToGPU();
}

// could be excecuted on a thread!
// the data is being held by the Texture, you can't deleted after calling this method. It will be deleted when you call the method unload or when you destroy the Texture
void Texture::loadCPU(unsigned char* data, const unsigned int &w, const unsigned int &h, const unsigned int &bpp, bool* doned)
{
    std::lock_guard<std::mutex> l(_mtx);
    if (doned)
        *doned = false;
    _cpuLoaded = false;
    _width = w;
    _height = h;
    _bpp = bpp;
    _bpc = img::bpcFromBpp(_bpp);
    _channels = img::nbOfChannelFromBpp(_bpp);

    int length = w*h*_channels*(_bpc/8);

    _data = data;
    if (!_data)
        return;
#ifdef mydebug
    textures_memory::_cpus ++;
#endif

    if (doned)
        *doned = true;
    _cpuLoaded = true;
}

void Texture::loadCPU(const char* filepath, bool* doned)
{
    _path = std::string(filepath);
    std::lock_guard<std::mutex> l(_mtx);
    if (!filepath)
    {
        lg("filepath is nullptr !");
        return;
    }

    if (doned)
        *doned = false;
    _cpuLoaded = false;
    if (_data)
        delete[] _data;
    _data = img::openGLFormatted(filepath, &_width, &_height, &_bpp);
    if (!_data)
        return;

#ifdef mydebug
    textures_memory::_cpus ++;
#endif

    _bpc = img::bpcFromBpp(_bpp);
    _channels = img::nbOfChannelFromBpp(_bpp);

    if (doned)
        *doned = true;
    _cpuLoaded = true;
}

void Texture::loadCPU(const char* filepath, boost::function<void ()> onDoned)
{
    this->loadCPU(filepath);
    onDoned();
}

void Texture::loadCPU(fipImage* i, boost::function<void ()> onDoned)
{
    {
        std::lock_guard<std::mutex> l(_mtx);
        if (!i)
        {
            lg("fipimage is nullptr !");
            return;
        }

        _cpuLoaded = false;
        if (_data)
            delete[] _data;
        _data = img::openGLFormatted(i, &_width, &_height, &_bpp);
        if (!_data)
            return;

#ifdef mydebug
        textures_memory::_cpus ++;
#endif

        _bpc = img::bpcFromBpp(_bpp);
        _channels = img::nbOfChannelFromBpp(_bpp);

        _cpuLoaded = true;
    }
    if (onDoned)
        onDoned();
}

void Texture::unload(const bool &cpu, const bool &gpu)
{
    std::lock_guard<std::mutex> l(_mtx);
    if (cpu && _data)
    {
        delete[] _data;
        _data = nullptr;

#ifdef mydebug
        textures_memory::_cpus --;
#endif
    }
    if (gpu)
    {
        gl_call(glDeleteTextures(1, &glId));
#ifdef mydebug
        if (_sendToGPUCalled)
            textures_memory::_gpus --;
#endif
    }
}

void Texture::bind(unsigned int slot )
{
    //gl_call(glActiveTexture(GL_TEXTURE0 + slot)); // not sure what it does... for now it make the profgram segfault
    gl_call(glBindTexture(GL_TEXTURE_2D, glId));
}

void Texture::unbind()
{
    gl_call(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture::sendToGPU()
{
    if (_sendToGPUCalled)
        return;
    _mtx.lock();
    this->glInit();
    this->bind();

    lg(_width << " x " << _height << " - " << _bpp <<" bits/pxl");

    gl_call(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    if (_bpp == 24)
    {
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, _data));
    }
    else if (_bpp == 32)
    {
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _data));
    }
    else if (_bpp == 48)
    {
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, _width, _height, 0, GL_RGB, GL_UNSIGNED_SHORT, _data));
    }
    else if (_bpp == 64)
    {
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, _width, _height, 0, GL_RGBA, GL_UNSIGNED_SHORT, _data));
    }
    else if (_bpp == 96)
    {
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _width, _height, 0, GL_RGB, GL_FLOAT, _data));
    }
    else if (_bpp == 128)
    {
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, _data));
    }

    //gl_call(glGenerateMipmap(GL_TEXTURE_2D)); // not sure whaat is it - make the program crash for now.
     this->unbind();
// 
     _gpuLoaded = true;
     _mtx.unlock();

#ifdef mydebug
        textures_memory::_gpus ++;
#endif
    _sendToGPUCalled = true;
}

