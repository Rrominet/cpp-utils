#include "random.h"
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include <chrono>
#include <random>

namespace mlrandom
{
    int _seed = 0;
}

void mlrandom::setSeed(const int &seed)
{
    srand(seed);
    _seed = seed;
}

int mlrandom::seed()
{
    return _seed;
}

double mlrandom::getFloat(const double &min, 
        const double &max)
{
    static std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}

int mlrandom::getInt(const int &min, const int &max)
{
    return rand() % (max - min) + min; 
}

double mlrandom::gaussian()
{
    double v1, v2, s; 
    do
    {
        v1 = 2.0f * mlrandom::getFloat() - 1.0f;
        v2 = 2.0f * mlrandom::getFloat() - 1.0f;
        s = v1* v1 + v2 * v2;
    }while(s>= 1.0f || s == 0.0f);

    s= sqrt((-2.0f * log(s))/ s);
    return v1 * s;
}

double mlrandom::gaussian(const double &mean, const double &deviation)
{
    return mean + mlrandom::gaussian() * deviation;
}

double mlrandom::gaussian(const double &mean, const double &deviation, const double &min, const double &max)
{
    double x; 
    do {
        x = mlrandom::gaussian(mean, deviation);
    }while (x < min || x > max);
    return x;
}

std::string mlrandom::id(const int &length)
{
    srand(std::chrono::system_clock::now().time_since_epoch().count());
    char ls[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    std::string res = "";
    for (int i=0; i<length; i++)
        res += ls[mlrandom::getInt(0, strlen(ls))];
    return res;
}
