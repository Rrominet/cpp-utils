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
            enum LoadType
            {
                DEFAULT,
                SYNC_NO_DECODE,
                SYNC_DECODE,
                ASYNC,
            };
            Sound(Sounds* manager, std::string path) : _manager(manager), _path(path) {}
            virtual ~Sound();

            ml::Ret<> init(LoadType type = DEFAULT);

            void play();
            void pause();
            bool playing() const;
            void setVolume(float vol);
            float volume() const;

            void toggle();

            double mspos();
            double mslength() const;
            void setMsPos(double pos);

            //0 = start, 1 = end
            void setPos(float ratio);

            long long unsigned pcmLength() const;
            long long unsigned pcmPos() const;
            void setPcmPos(long long unsigned pos);

            void setPan(float pan);
            float pan() const;

            void setPitch(float pitch);
            float pitch() const;

            void setLooping(bool looping);
            bool looping() const;

        private : 
            bool _initialized = false;
            ma_sound _sound; //bp cg
            Sounds* _manager; //bp cg
            std::string _path; //bp cgs

        public : 
#include "./Sound_gen.h"
    };
}
