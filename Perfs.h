#pragma once
#include <chrono>
using namespace std::chrono;

class Perfs
{
    public : 
        enum Unit
        {
            NANO = 1, 
            MICRO, 
            MILLI,
            S, // S for seconds
        };
        Perfs(){}
        ~Perfs(){}

        void start();
        void end();

        double duration(const Unit& unit=MILLI);

        //check if at least 16ms passed since last call of start(), you can call this function several time
        bool hasPassed(float ms);
        bool has16msPassed();
        double passed(const Unit& unit=MILLI);

    private : 
        steady_clock::time_point _start ;
        steady_clock::time_point _end ;

};

