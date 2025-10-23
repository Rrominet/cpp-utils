#pragma once
#include <mutex>
#include <boost/function.hpp>
#include <atomic>

class fipImage;

class Texture
{
    private :
        std::string _path;
        unsigned char* _data = nullptr;
        unsigned int _width, _height, _bpc, _bpp, _channels;
        unsigned int glId = 0;

        void glInit();
        bool _cpuLoaded = false;
        bool _gpuLoaded = false;

        std::mutex _mtx;
        std::atomic_bool _sendToGPUCalled = false;

    public :
        Texture ();
        // all the constructors can't be called from threads, use the method loadCPU for this
        Texture (unsigned char* data, const unsigned int &w, const unsigned int &h, const unsigned int &bpp = 24);
        Texture (fipImage* img);
        Texture(const char* filepath, bool sendToGPU=true);
        ~Texture();

        std::string path() const {return _path;}

        void load(unsigned char* data, const unsigned int &w, const unsigned int &h, const unsigned int &bpp = 24);
        //could be executed on a thread
        // the data is being held by the Texture, you can't deleted after calling this method. It will be deleted when you call the method unload or when you destroy the Texture
        void loadCPU(unsigned char* data, const unsigned int &w, const unsigned int &h, const unsigned int &bpp = 24, bool* doned=nullptr);
        //could be executed on a thread
        void loadCPU(const char* filepath, bool* doned=nullptr);
        void loadCPU(const char* filepath, boost::function<void ()> onDoned);
        void loadCPU(const std::string& filepath){this->loadCPU(filepath.c_str());};
        void loadCPU(fipImage* i, boost::function<void ()> onDoned=0);

        template <typename H>
        void loadCPU(const char* filepath, boost::function<void ()> onDoned, H* handler)
        {
            this->loadCPU(filepath);
            handler->execNextFrame([=]{onDoned();});
        }


        //CAN'T be executed on a a different thread than the OPENGL Ctx one
        void sendToGPU();

        // destroy the data associated with the texture on the CPU and GPU
        void unload(const bool &cpu=true, const bool &gpu=true);
        void bind(unsigned int slot = 0);
        void unbind();

        unsigned char* data()
        {
            const std::lock_guard<std::mutex> lock(_mtx);
            return _data;
        }
        unsigned int width() const {return _width;}
        unsigned int height() const {return _height;}
        unsigned int bpc() const {return _bpc;}
        unsigned int bpp() const {return _bpp;}
        unsigned int w() const {return _width;}
        unsigned int h() const {return _height;}
        unsigned int id()const {return glId;}
        unsigned int channels()const {return _channels;}

        bool cpuLoaded() const {return _cpuLoaded;}
        bool gpuLoaded() const {return _gpuLoaded;}
};

namespace textures_memory
{
    int countAlive();
    int cpuAlive();
    int gpuAlive();
}
