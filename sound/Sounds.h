#pragma once
#include "../vec.h"
#include "miniaudio.h"
#include "../Ret.h"

namespace ml
{
    class Sound;
    class Sounds
    {
        public : 
            Sounds();
            virtual ~Sounds();
            
            ml::Ret<ml::Sound*> play(const std::string& path);
            ml::Ret<ml::Sound*> at(size_t index);

        private : 
            bool _initialized = false;
            ma_engine _engine; //bp cg
            ml::Ret<> _init();
            ml::Vec<std::unique_ptr<ml::Sound>> _sounds;

        public : 
            #include "./Sounds_gen.h"
    };
}
