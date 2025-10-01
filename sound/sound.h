#pragma once
#include <string>
#include "mlSystem.h"
#include "../thread.h"
#include "../Events.h"

class mlSystem;
class mlSound;

namespace ml
{
    namespace sound
    {
        mlSystem* system();
        mlSound* load(const std::string& filepath);

        // the sound is created but you don't neet to delete it. The system() will
        void async_load(const std::string& filepath, boost::function <void (mlSound*)> onDoned=0);
        mlSound* play(const std::string& filepath, const mlSystem::MemoryRead& mem=mlSystem::UNBLOCKING);

        // do not delete it with delete !
        FMOD::DSP* addFx(FMOD_DSP_TYPE type, FMOD::Channel* ch);
        FMOD::DSP* copyFx(FMOD::DSP* fx, mlSystem* sys, FMOD::Channel* ch);
        void removeFx(FMOD::DSP* dsp, FMOD::Channel* ch);
        std::vector<std::unique_ptr<FMOD::DSP>>& fxs();

        void Export(const std::string& path, mlSound* sound);
        ml::Events& events();
    }
}
