#pragma once 

#include <string>
#include <vector>

#include <fmod_errors.h>
#include <fmod_dsp.h>
#include <fmod_dsp_effects.h>
#include <fmod.hpp>

#include <boost/function.hpp>
#include <atomic>
#include "Events.h"

struct SoundData
{
    void* data= nullptr;
    unsigned long long length; // in Byte  (1 char unit)
    FMOD_SOUND_TYPE   type;
    FMOD_SOUND_FORMAT format;
    //bits per channel
    int               bits;
    int               channels;

    char* getData(){return (char*)this->data;};

    //begin and offset are in bytes !
    std::vector<float> dataPart(long long unsigned begin=0, long long unsigned end = 0, unsigned int step = 1);
    std::vector<float> dataPart2(long long unsigned begin=0, long long unsigned offset = 1, unsigned int step = 1);

    ~SoundData();

    // a sample is one sound data unit (8bit, 16bit, 24bit, or 32bits depending of t
    // it's converted in a float for easier treatement. (between 0 and 1)
    void processOnOneSample(const boost::function <void (float)>& f, unsigned long long offset=0);
    //
    //f function will be executed for every samples until length.
    //length and offset are always in Byte
    //step is in sample unit not in byte
    void processOnEverySamples(const boost::function <void (float)>& f, unsigned long long length=0, unsigned long long offset=0, unsigned int step=1, unsigned int* currentStep = nullptr);
};

class mlSystem;
class mlSound
{
    private : 
        FMOD_RESULT res;
        std::string _path;
        std::atomic<bool> _loading = false;
        float _speed = 1.0;
        ml::Events _events;

    public : 
        FMOD::Sound* _sound = nullptr; 
        FMOD::Channel* _ch = nullptr;
        mlSystem* _system = nullptr;
        std::vector<FMOD::DSP*> effects;
        bool _reversed = false;
        float _volume = 1.0;
        bool _playing = false;
        float _baseFreq = 44000.0;

        mlSound(const std::string &path, mlSystem* sys,
                int mem=1, // ml::System::BLOCKING
                FMOD_MODE mode = FMOD_DEFAULT, 
                FMOD_CREATESOUNDEXINFO *exinfo=nullptr);
        mlSound(const char* data, mlSystem* sys, FMOD_CREATESOUNDEXINFO *exinfo);
        ~mlSound();
        mlSound* copy(mlSystem* sys = nullptr);

        FMOD::Sound* sound() const{return _sound;}

        FMOD::Channel* channel() const {return _ch;}
        FMOD::Channel* ch() const {return _ch;}

        mlSystem* system()const {return _system;}
        mlSystem* sys() const {return _system;}
        bool reversed() const {return _reversed;}

        bool play();
        bool playing();
        bool preloadChannel();
        bool pause();
        bool restart();
        bool toggle();

        bool mute();
        bool unmute();
        bool toggleMute();
        bool setPan(const float &pan);
        bool setPitch(const float &p);

        float pitch();

        bool muted();

        bool setVolume(float vol);
        bool addVolume(float vol) {return this->setVolume(this->volume() + vol);}
        bool setVolumeDb(float vol);
        float volume();
        float volumeDb();

        bool reverse();
        float freq();
        bool setFreq(float f);
        bool setSpeed(float f);

        float samplesNumber(int ms=-1);

        // in PCM UNIT / Sample - carefull here it's the length of ONE channel, if yout want the total length use totalLength method
        unsigned int length();
        unsigned int totalLength();
        unsigned int msLength();
        unsigned int byteLength();
        unsigned int position();
        unsigned int bytePosition();
        // bits is the number of bit neede to create a sound sample (8, 16, 24, or 32)
        unsigned int byteSize(unsigned int ms);
        unsigned int msposition();
        float positionRatio();
        bool setPosition(const unsigned int& p);
        bool setPositionRatio(const float &val);
        bool setMsposition(const unsigned int& p);

        bool movePrevious(const unsigned int& p);
        bool moveMsPrevious(const unsigned int& p);

        bool moveNext(const unsigned int& p);
        bool moveMsNext(const unsigned int& p);

        void loop();
        void unloop();
        bool looped();

        bool finished();

        FMOD::DSP* newEffect(FMOD_DSP_TYPE type);
        static SoundData format(FMOD::Sound* s);
        static unsigned int length(FMOD::Sound* s);
        std::string path() const {return _path;}
        void setPath(const std::string &path){_path = path;}

        // careful here SoundData->data is nullptr, if you want the sound data as byte you need to use mlSound::data()
        SoundData format() const {return mlSound::format(this->sound());}

        // this SoundData must be deleted by yourself ! (it will not be deleted by the mlSound)
        SoundData* data(unsigned int offset=0, 
                unsigned int length = 0);

        //thread safe
        bool loading() const {return _loading;}

        //nmber of char in the data
        long memory();

        float speed()const {return _speed;}
        ml::Events& events() {return _events;}
};
