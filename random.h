#pragma once
#include <string>
#include <time.h>

namespace mlrandom
{
    void setSeed(const int &seed);
    int seed();
    double getFloat(const double &min=0.0, const double &max=1.0);
    int getInt(const int &min=0, const int &max=100);

    // utlity function for the gaussian one, should not be used alone..
    double gaussian();
    double gaussian(const double &mean, const double &deviation);
    double gaussian(const double &mean, const double &deviation, const double &min, const double &max);

    // be careful with that it change the global random seed
    std::string id(const int &length=10);
};
