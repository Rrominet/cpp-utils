#pragma once
#include "../vec.h"
#include "miniaudio.h"
#include "../Ret.h"
#include <cstddef>

namespace ml
{
    namespace sound
    {
        enum SoundFormat
        {
            UNKNOWN,
            U8,
            S16,
            S24,
            S32,
            F32
        };
    }

    struct SoundData
    {
        sound::SoundFormat format = sound::UNKNOWN;
        unsigned int channels = 0;
        unsigned int sampleRate = 0;
        ml::Vec<ml::Vec<float>> channelsData;

        double mslength() const;
        long long unsigned pcmLength() const;
    };

    class Sound;
    class Sounds
    {
        public : 
            Sounds();
            virtual ~Sounds();
            
            ml::Ret<ml::Sound*> play(const std::string& path);
            ml::Ret<ml::Sound*> at(size_t index);

            ml::Ret<SoundData> data(size_t index);
            ml::Ret<SoundData> data(ml::Sound* sound);
            ml::Ret<SoundData> data(const std::string& filepath);

        private : 
            bool _initialized = false;
            ma_engine _engine; //bp cg
            ml::Ret<> _init();
            ml::Vec<std::unique_ptr<ml::Sound>> _sounds;

        public : 
            #include "./Sounds_gen.h"
    };
}
