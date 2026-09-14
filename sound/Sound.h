#pragma once
#include <string>
#include "Ret.h"
#include "miniaudio.h"

namespace ml
{
    class Sounds;
    class Sound
    {
        public : 
            Sound(Sounds* manager, std::string path) : _manager(manager), _path(path) {}
            virtual ~Sound();

            ml::Ret<> init();

            void play();
            void pause();
            bool playing() const;
            void setVolume(float vol);
            float volume() const;

            void toggle();

        private : 
            bool _initialized = false;
            ma_sound _sound; //bp cg
            Sounds* _manager; //bp cg
            std::string _path; //bp cgs

        public : 
#include "./Sound_gen.h"
    };
}
