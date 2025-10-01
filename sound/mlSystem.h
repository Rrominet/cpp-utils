#pragma once 
#include <fmod.hpp>
#include <fmod_errors.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include "vec.h"
#include "mlSound.h"

class Sounds;
class mlSystem{
    protected : 
        std::string _out;
        ml::Vec<std::unique_ptr<mlSound>> tmps;
        FMOD::System * _fmod = nullptr;
        std::mutex _mtx;

    public : 
        enum MemoryRead
        {
            BLOCKING = 1,
            UNBLOCKING,
        };

        mlSystem(FMOD::System* sys) : tmps(){_fmod = sys;}
        ~mlSystem() = default;

        ml::Vec<std::unique_ptr<mlSound>>& sounds(){return tmps;}
        std::string out(){return _out;}

        static std::unique_ptr<mlSystem> createAndInit(
                const char* filename = nullptr, 
                FMOD_OUTPUTTYPE output = FMOD_OUTPUTTYPE_WAVWRITER_NRT, 
                const int &mxChannels = 512, 
                FMOD_INITFLAGS flags = FMOD_INIT_NORMAL, 
                void *extradriverdata = 0);

        FMOD::System* fmod();
        mlSound* sound(const std::string &path,
                const MemoryRead &mem=UNBLOCKING,
                FMOD_MODE mode = FMOD_DEFAULT, 
                FMOD_CREATESOUNDEXINFO *exinfo=nullptr);
        mlSound* soundFromData(const char* data, FMOD_CREATESOUNDEXINFO *exinfo);
        
        // the system take ownership of the pointer here.
        // do not delete it yourself.
        void addSound(mlSound* s);
        void removeSound(mlSound* s);

        void clearSounds();
        FMOD_RESULT release();
        unsigned int length();
        void Export();
};
