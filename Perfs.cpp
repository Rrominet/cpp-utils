#include "Perfs.h"

void Perfs::start()
{
    _start = steady_clock::now();
}
void Perfs::end()
{
    _end = steady_clock::now();
}

double Perfs::duration(const Unit& unit)
{
    auto res = duration_cast<nanoseconds>(_end - _start).count();
    if (unit == MICRO)
        return res * 0.001;
    else if (unit == MILLI)
        return res * 0.000001;
    else if (unit == S)
        return res * 0.000000001;
    return 0.0;
}

bool Perfs::hasPassed(float ms)
{
    this->end();
    if (this->duration()>=ms)
        return true;
    return false;
}

bool Perfs::has16msPassed()
{
    return this->hasPassed(16);
}

double Perfs::passed(const Unit& unit)
{
    this->end();
    return this->duration(unit);
}
