#include "img_utils.h"
#include "FreeImage.h"
#include "debug.h"
#include "mlMath.h"
#include <omp.h>
#include "Events.h"
#include "files.2/files.h"

namespace bp = boost::process;

namespace img
{
    ml::Events _events;
    ml::Events& events() {return _events;}
}



using namespace std; 
fipImage* img::copy(const fipImage &i)
{
    fipImage* _r = new fipImage(i); 
    return _r;
}

fipImage* img::copy(const string &path)
{
    fipImage ref; 
    ref.load(path.c_str());
    fipImage* _r =  img::copy(ref);
    ref.clear(); 
    return _r;
}

bool img::copy(const string &i, const string dest)
{
    fipImage ref1; 
    ref1.load(i.c_str()); 
    fipImage ref2(ref1);
    bool _r =  ref2.save(dest.c_str());
    ref1.clear(); ref2.clear();
    return _r;
}

bool img::convert(const fipImage &i, const string &dest, const int &flags, const int resize)
{
    std::string ext = files::ext(dest);
    boost::to_lower(ext);
    fipImage conv(i); 
    if (ext == "jpg"||ext == "jpeg") 
        conv.convertTo24Bits(); 
    else if (ext == "webp" && conv.isTransparent())
        conv.convertTo32Bits();
    else if (ext == "webp") 
        conv.convertTo24Bits();
    if (resize != -1)
    {
        float ratio = conv.getHeight()*1.0/conv.getWidth()*1.0;
        conv.rescale(resize, resize*ratio, FILTER_LANCZOS3);
    }
    bool r =  conv.save(dest.c_str(), flags);
    conv.clear(); 
    return r;
}

bool img::convert(const string &src, const string &dest, const int &flags, const int resize)
{
    string ext = files::ext(dest);
    boost::to_lower(ext);
    if (ext=="svg")
    {
        std::string pdf = files::parent(src) + files::sep() + files::baseName(src) + ".pdf";
        bp::system("epstopdf \"" + src + "\" \"" + pdf + "\"");
        bp::system("pdf2svg \"" + pdf + "\" \"" + dest + "\"");
        files::remove(pdf);
        return true;
    }
    fipImage srcImg; 
    srcImg.load(src.c_str());

    lg("convert " + src + " to " + dest);
    bool _r =  img::convert(srcImg, dest, flags, resize);
    srcImg.clear(); 
    return _r;
}

#ifdef _USING_WX_
wxImage* img::asWx(fipImage i)
{
    lg2("Red offset", FI_RGBA_RED);
    lg2("Green offset", FI_RGBA_GREEN);
    lg2("Blue offset", FI_RGBA_BLUE);
    lg2("Alpha offset", FI_RGBA_ALPHA);

    int pxlsize = 3;

    if (i.isTransparent()) 
    {
        lg("img contain transparent channel");
        i.convertTo32Bits();
        pxlsize = 4;
    }
    else 
        i.convertTo24Bits();
    if (!i.accessPixels())
        return new wxImage(i.getWidth(), i.getHeight());

    lg2("Pixl size", pxlsize);

    int count = i.getWidth() * i.getHeight()*3;
    unsigned char* bytes = (unsigned char*) malloc(count * sizeof(unsigned char));
    int k = 0;

    if (!i.isTransparent())
    {
        for (int y=i.getHeight()-1; y>=0; y--)
        {
            BYTE* pxls = i.getScanLine(y);
            for (int x=0; x<i.getWidth()*pxlsize; x+=pxlsize)
            {
                bytes[k] = pxls[x + FI_RGBA_RED];
                bytes[k+1] = pxls[x + FI_RGBA_GREEN];
                bytes[k+2] = pxls[x + FI_RGBA_BLUE];
                k+=3;
            }
        }
        lg("pixels copied.");
        return new wxImage(i.getWidth(), i.getHeight(), bytes);
    }
    else 
    {
        unsigned char* alpha = (unsigned char*) malloc(i.getWidth()*i.getHeight() * sizeof(unsigned char));
        int inca = 0;
        for (int y=i.getHeight()-1; y>=0; y--)
        {
            BYTE* pxls = i.getScanLine(y);
            for (int x=0; x<i.getWidth()*pxlsize; x+=pxlsize)
            {
                bytes[k] = pxls[x + FI_RGBA_RED];
                bytes[k+1] = pxls[x + FI_RGBA_GREEN];
                bytes[k+2] = pxls[x + FI_RGBA_BLUE];
                k+=3;

                alpha[inca] = pxls[x + FI_RGBA_ALPHA];
                inca ++;
            }
        }
        lg("pixels copied.");
        lg("alpha copied.");

        return new wxImage(i.getWidth(), i.getHeight(), bytes, alpha);
    }
    return new wxImage(i.getWidth(), i.getHeight());
}

wxImage* img::asWx(const std::string& img)
{
    fipImage i;
    i.load(img.c_str());
    return img::asWx(i);
}
#endif

unsigned char* img::openGLFormatted(const char* filepath, unsigned int* w, unsigned int* h, unsigned int* bpp)
{
    fipImage _img;
    bool res = _img.load(filepath);
    if (!res)
        return nullptr;

    return img::openGLFormatted(&_img, w, h, bpp);
}

unsigned char* img::openGLFormatted(fipImage* i, unsigned int* w, unsigned int* h, unsigned int* bpp)
{
    if (i->getBitsPerPixel() != 24 || i->getBitsPerPixel() != 32)
    {
        if (!i->isTransparent())
            i->convertTo24Bits();
        else 
            i->convertTo32Bits();
    }

    if (!i->accessPixels())
        return nullptr;

    *bpp = i->getBitsPerPixel();

    int len = i->getWidth()*i->getHeight()*(*bpp/8);
    unsigned char* data = new unsigned char [len];
    int bytesspp = i->getLine()/i->getWidth();
    int j= 0;
    for (int y=i->getHeight()-1; y>=0; y--) // y-- because we need to reverse the fip data on y axe.
    {
        BYTE* bits = i->getScanLine(y);
        for (unsigned int x=0; x<i->getWidth(); x++)
        {
            data[j] = bits[FI_RGBA_RED];
            data[j+1] = bits[FI_RGBA_GREEN];
            data[j+2] = bits[FI_RGBA_BLUE];
            if (bytesspp == 4)
                data[j+3] = bits[FI_RGBA_ALPHA];

            j += bytesspp;
            bits += bytesspp;
        }
    }

    *w = i->getWidth();
    *h = i->getHeight();
    return data;
}

void img::bgrToRgb(fipImage* i)
{
    unsigned int w = i->getWidth();
    unsigned int h = i->getHeight();
    unsigned int pitch = i->getScanWidth();
    unsigned int bpp = i->getBitsPerPixel();

    FREE_IMAGE_TYPE type = i->getImageType();

    if (type != FIT_BITMAP)
        return;
    BYTE *bits = (BYTE*)i->accessPixels();

    //CAREFULL with that - it could depends pf the OS. Need to check if it's ok on windows and OWs too...
    if (bpp == 24)
    {
        for (int y=0; y<i->getHeight(); y++)
        {
            BYTE* px = (BYTE*) bits;
            for (int x = 0; x<i->getWidth(); x++)
            {
                BYTE tmp = px[0];
                px[0] = px[2];
                px[2] = tmp;
                px += 3;
            }
            bits += pitch;
        }
    }
    else if (bpp == 32)
    {
        for (int y=0; y<i->getHeight(); y++)
        {
            BYTE* px = (BYTE*) bits;
            for (int x = 0; x<i->getWidth(); x++)
            {
                BYTE tmp = px[0];
                px[0] = px[2];
                px[2] = tmp;
                px += 4;
            }
            bits += pitch;
        }
    }
}

unsigned int img::bpcFromBpp(const unsigned &bpp)
{
    unsigned int bpc = 0;
    if (bpp == 1 || bpp == 3 || bpp == 4)
        bpc = 1;
    else if (bpp == 6 || bpp == 8)
        bpc = 2;
    else if (bpp == 24 || bpp == 32)
        bpc = 8;
    else if (bpp == 48 || bpp == 64)
        bpc = 16;
    else if (bpp == 96 || bpp == 128)
        bpc = 32;

    return bpc;
}

unsigned int img::nbOfChannelFromBpp(const unsigned &bpp)
{
    unsigned int channels = 0;
    if (bpp == 1 || bpp == 6 || bpp == 8)
        channels = 1;
    else if (bpp == 24 || bpp == 48 || bpp == 96)
        channels = 3;
    else if (bpp == 32 || bpp == 64 || bpp == 128)
        channels = 4;
    return channels;
}

void img::setOnAllPixels(fipImage* img, const boost::function<void(int, int, int, float*, float*, float*, float*)>& f)
{
    bool transparent = img->isTransparent();
    // 
    if (img->getBitsPerPixel() == 24 || img->getBitsPerPixel() == 32)
    {
        for (int y=0; y<img->getHeight(); y++)
        {
            BYTE* bits = (BYTE *)img->getScanLine(y);
            for (int x=0; x<img->getWidth(); x++)
                _setOnPixel(x, y, img->getHeight(), bits, transparent, 255, f);
        }
    }

    else if (img->getImageType() == FIT_RGB16 || img->getImageType() == FIT_RGBA16)
    {
        for (int y=0; y<img->getHeight(); y++)
        {
            unsigned short* bits = (unsigned short *)img->getScanLine(y);
            for (int x=0; x<img->getWidth(); x++)
                _setOnPixel(x, y, img->getHeight(), bits, transparent, 65535, f);
        }
    }

    else if (img->getImageType() == FIT_RGBF || img->getImageType() == FIT_RGBAF)
    {
#pragma omp parallel for
        for (int y=0; y<img->getHeight(); y++)
        {
            float* bits = (float *)img->getScanLine(y);
            for (int x=0; x<img->getWidth(); x++)
                _setOnPixel(x, y, img->getHeight(), bits, transparent, 1.0f, f);
        }
    }
}

fipImage img::add(const fipImage &a, const fipImage &b, float minClamp, float maxClamp)
{
    auto _a = asFloat(a);
    auto _b = asFloat(b);
    auto f = [=](int x, int y, int offset, float*r, float*g, float*b, float*a)
    {
        // float work here because I converted the img in 32 bits floats before !
        float* ap = img::pixel(_a, x, y);
        float* bp = img::pixel(_b, x, y);

        *r = ap[FI_RGBA_RED] + bp[FI_RGBA_RED];
        *g = ap[FI_RGBA_GREEN] + bp[FI_RGBA_GREEN];
        *b = ap[FI_RGBA_BLUE] + bp[FI_RGBA_BLUE];
        if (a)
        {
            *a = ap[3] + bp[3];
            math::clamp(a, minClamp, maxClamp);
        }
        math::clamp(r, minClamp, maxClamp);
        math::clamp(g, minClamp, maxClamp);
        math::clamp(b, minClamp, maxClamp); 
    };

    auto _img = fipImage(_a);
    img::setOnAllPixels(&_img, f);
    return _img;
}

fipImage img::asFloat(const fipImage& img)
{
    fipImage _r(img); 
    _r.convertToRGBAF();
    return _r;
}

fipImage img::as8bits(const fipImage& img)
{
    fipImage _r; 
    if (img.isTransparent())
    {
        _r.setSize(FIT_BITMAP, img.getWidth(), img.getHeight(), 32);
        for (int y=0; y<_r.getHeight(); y++)
        {
            BYTE* bits = (BYTE *)_r.getScanLine(y);
            for (int x=0; x<_r.getWidth(); x++)
            {
                bits[FI_RGBA_ALPHA] = 255;
                bits += 4;
            }
        }

    }
    else 
        _r.setSize(FIT_BITMAP, img.getWidth(), img.getHeight(), 24);

    for (int y=0; y<_r.getHeight(); y++)
    {
        BYTE* bits = (BYTE *)_r.getScanLine(y);
        float* floats = (float*)img.getScanLine(y);
        for (int x=0; x<_r.getWidth(); x++)
        {
            bits[FI_RGBA_RED] = 255.0 * floats[FI_RGBA_RED];
            bits[FI_RGBA_GREEN] = 255.0 * floats[FI_RGBA_GREEN];
            bits[FI_RGBA_BLUE] = 255.0 * floats[FI_RGBA_BLUE];

            if (img.isTransparent())
            {
                bits[FI_RGBA_ALPHA] = 255.0 * floats[FI_RGBA_ALPHA];
                bits += 4;
                floats += 4;
            }
            else 
            {
                bits += 3;
                floats += 3;
            }
        }

    }

    return _r;
}

fipImage img::as16bits(const fipImage& img)
{
    fipImage _r; 
    if (img.isTransparent())
    {
        _r.setSize(FIT_RGBA16, img.getWidth(), img.getHeight(), 48);
        for (int y=0; y<_r.getHeight(); y++)
        {
            unsigned short* bits = (unsigned short *)_r.getScanLine(y);
            for (int x=0; x<_r.getWidth(); x++)
            {
                bits[FI_RGBA_ALPHA] = 65535;
                bits += 4;
            }
        }
    }
    else 
        _r.setSize(FIT_UINT16, img.getWidth(), img.getHeight(), 64);

    for (int y=0; y<_r.getHeight(); y++)
    {
        unsigned short* bits = (unsigned short *)_r.getScanLine(y);
        float* floats = (float*)img.getScanLine(y);
        for (int x=0; x<_r.getWidth(); x++)
        {
            bits[FI_RGBA_RED] = 65535.0 * floats[FI_RGBA_RED];
            bits[FI_RGBA_GREEN] = 65535.0 * floats[FI_RGBA_GREEN];
            bits[FI_RGBA_BLUE] = 65535.0 * floats[FI_RGBA_BLUE];

            if (img.isTransparent())
            {
                bits[FI_RGBA_ALPHA] = 65535.0 * floats[FI_RGBA_ALPHA];
                bits += 4;
                floats += 4;
            }
            else 
            {
                bits += 3;
                floats += 3;
            }
        }
    }

    return _r;
}

float* img::pixel (const fipImage& _RGBAF, int x, int y)
{
    int fipY = _RGBAF.getHeight() - y - 1;
    int pitch = _RGBAF.getScanWidth();
    BYTE *bits = (BYTE*)_RGBAF.accessPixels();
    float* currentLine = (float*)(&bits[pitch*fipY]);
    return &currentLine[x*4];
}


void img::addAll(const std::vector<std::string>& seq, const std::string& destDir, float minClamp, float maxClamp)
{
    if (!files::isDir(destDir))
        files::mkdir(destDir);
    for (int i=0; i<seq.size(); i++) 
    {
        fipImage fip;
        fip.load(seq[i].c_str());
        std::string out = destDir + files::sep() + files::name(destDir) + "." + str::pad(i) + "." + files::ext(seq[i]);
        if (i == 0)
        {
            if (fip.save(out.c_str()))
                lg("image " << i << " doned.");
            else 
                lg("Could not save image " << i);
            continue;
        }

        fipImage lastFip;
        std::string lastDonedPath = destDir + files::sep() + files::name(destDir) + "." + str::pad(i-1) + "." + files::ext(seq[0]);
        lastFip.load(lastDonedPath.c_str());
        fipImage res = img::add(lastFip, fip);
        if (fip.getBitsPerPixel() == 24 || fip.getBitsPerPixel() == 32)
            res = img::as8bits(res);
        else if (fip.getBitsPerPixel() == 48 || fip.getBitsPerPixel() == 64)
            res = img::as16bits(res);

        lg2("fip bpp", fip.getBitsPerPixel());
        if (files::ext(seq[i]) == "jpg" && fip.getBitsPerPixel() == 24)
            res.convertTo24Bits();
        lg2("res bpp", res.getBitsPerPixel());
        if (res.save(out.c_str()))
            lg("image " << i << " doned.");
        else 
            lg("Could not save image " << i);
    }
}

void img::rescale(fipImage& img, float width, float height )
{
    if (height == -1)
        height = width * img.getHeight() / img.getWidth();

    img.rescale(width, height, FILTER_LANCZOS3);
}

    // path could be a dir, in this case, the dest must be a dir. And in this case, all image in it will be scaled and wrtieen in the dest dir
void img::rescale(const std::string& path, const std::string& dest, float width, float height )
{
    if (!files::exists(path))
        throw std::runtime_error("File " + path + " does not exist, can't rescale it.");
    if (!files::isImg(path) && files::isDir(path))
        throw std::runtime_error("File " + path + " is not an image or a directory, can't rescale it.");

    if (files::isImg(path))
    {
        if (!files::isImg(dest))
            throw std::runtime_error("File destination : " + dest + " is not an image, can't write the rescaled image.");
        fipImage img;
        img.load(path.c_str());
        img::rescale(img, width, height);
        img.save(dest.c_str());
        img::events().emit("rescaled");
    }

    else if (files::isDir(path))
        img::rescale(files::ls(path), dest, width, height);
}


void img::rescale(const std::vector<std::string>& paths, const std::string& dest, float width, float height )
{
    if (files::exists(dest) && !files::isDir(dest))
        throw std::runtime_error("File destination : " + dest + " is not a directory, can't write the rescaled images in it.");

    if (!files::isDir(dest))
        files::mkdir(dest);

    int index = 0;
    for (const std::string& p : paths)
    {
        if (files::isImg(p))
        {
            fipImage img;
            img.load(p.c_str());
            img::rescale(img, width, height);
            const std::string out = dest + files::sep() + files::baseName(p) + "." + files::ext(p);
            img.save(out.c_str());
            index ++;
            img::events().emit("rescaled", index);
        }
    }
}
