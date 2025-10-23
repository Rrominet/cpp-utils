#include "geometry.h"

geometry::Size::Size(int x, int y)
{
    w = x;
    if (y == -1)
        h = w;
    else 
        h = y;
}

json geometry::Size::serialize()
{
    json _r;
    _r["w"] = this->w;
    _r["h"] = this->h;

    return _r;
}

void geometry::Size::deserialize(const json &data)
{
   this->w = data["w"] ;
   this->h = data["h"] ;
}
