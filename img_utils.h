#pragma once 

#include "Events.h"
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/function.hpp> //need to be included BEFORE windows.h

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _USING_WX_
#include <wx/image.h>
#endif

#include <string>
#include <FreeImage.h>
#include <FreeImagePlus.h>

namespace img
{
    using namespace std;

    fipImage* copy(const fipImage &i);
    fipImage* copy(const string &path);
    bool copy(const string &i, const string dest);
    bool convert(const fipImage &i, const string &dest, const int &flags = 0, const int resize=-1);
    bool convert(const string &src, const string &dest, const int &flags = 0, const int resize=-1);

#ifdef _USING_WX_
    // you need to delete the returned pointer yourself
    wxImage* asWx(fipImage i);
    wxImage* asWx(const std::string& img);
#endif

    unsigned char* openGLFormatted(const char* filepath, unsigned int* w, unsigned int* h, unsigned int* bpp);
    unsigned char* openGLFormatted(fipImage* i, unsigned int* w, unsigned int* h, unsigned int* bpp);
    void bgrToRgb(fipImage* i);
    unsigned int bpcFromBpp(const unsigned &bpp);
    unsigned int nbOfChannelFromBpp(const unsigned &bpp);

    // int args are the width and the height of the pixel (starting to 0, not 1) (top to bottom - left to right)
    // 3th int is the data offset of the data (R, G, B, A, R, G, B , A, R...) offset 
    // float args are R, G, B , A (in a 0 to 1 range)
    void setOnAllPixels(fipImage* img, const boost::function<void(int, int, int, float*, float*, float*, float*)> &f);

    template<typename P>
    void _setOnPixel(const int &x, const int &y, const int &height, P*& pixels, const bool &transparent, int maxPixelVal, const boost::function<void(int, int, int, float*, float*, float*, float*)>& f)
    {
        float r; float g; float b; float a;
        if (transparent)
            f(x, height - y - 1, x*y*sizeof(P), &r, &g, &b, &a);
        else 
            f(x, height - y - 1, x*y*sizeof(P), &r, &g, &b, nullptr);

        if (maxPixelVal == 255)
        {
            pixels[FI_RGBA_RED] = (float)maxPixelVal * r;
            pixels[FI_RGBA_GREEN] = (float)maxPixelVal * g;
            pixels[FI_RGBA_BLUE] = (float)maxPixelVal * b;
        }
        else 
        {
            pixels[0] = (float)maxPixelVal * r;
            pixels[1] = (float)maxPixelVal * g;
            pixels[2] = (float)maxPixelVal * b;
        }

        if (transparent)
        {
            if (maxPixelVal == 255)
                pixels[FI_RGBA_ALPHA] = (float)maxPixelVal * a;
            else 
                pixels[3] = (float)maxPixelVal * a;
            pixels += 4;
        }
        else 
            pixels += 3;
    }

    //convert to RGBA 32Bit per channel
    fipImage asFloat(const fipImage& img);

    // the fipImage as arg must be a 32 bit float img
    fipImage as8bits(const fipImage& img);
    // the fipImage as arg must be a 32 bit float img
    fipImage as16bits(const fipImage& img);

    float* pixel (const fipImage& _RGBAF, int x, int y);

    fipImage add(const fipImage &a, const fipImage &b, float minClamp=0.0, float maxClamp=1.0);

    // need to make it multithreaded !
    void addAll(const std::vector<std::string>& seq, const std::string& destDir, float minClamp=0.0, float maxClamp=1.0);

    void rescale(fipImage& img, float width, float height = -1);

    // path could be a dir, in this case, the dest must be a dir. And in this case, all image in it will be scaled and wrtieen in the dest dir
    void rescale(const std::string& path, const std::string& dest, float width, float height = -1);
    void rescale(const std::vector<std::string>& paths, const std::string& dest, float width, float height = -1);

    ml::Events& events();
};
