#include "sound.h"
#include "./mlSound.h"
#include "../vec.h"
#include "../files.2/files.h"
#include <memory>

namespace ml
{
    namespace sound
    {
        std::mutex _mtx;
        std::unique_ptr<mlSystem> _system;
        std::vector<std::unique_ptr<FMOD::DSP>> _fxs;
        ml::Events _events;
        mlSystem* system()
        {
            {
                if (!_system)
                {
                    _system = mlSystem::createAndInit(nullptr, 
                            FMOD_OUTPUTTYPE_AUTODETECT);
                    lg2("New sound system created", _system.get());
                }
            }
            return _system.get();
        }

        mlSound* load(const std::string& filepath)
        {
            return system()->sound(filepath.c_str(), mlSystem::BLOCKING);
        }

        void async_load(const std::string& filepath, boost::function <void (mlSound*)> onDoned)
        {
            auto f = [filepath, onDoned](){
                auto s = sound::load(filepath);
                if (onDoned)
                    onDoned(s);
            }; 

            threads::launch(f);
        }

        mlSound* play(const std::string& filepath, const mlSystem::MemoryRead& mem)
        {
            mlSound* s = nullptr;
            {
                th_guard(_mtx);
                s = system()->sound(filepath.c_str(), mem);
            }
            s->play();
            return s;
        }

        FMOD::DSP* addFx(FMOD_DSP_TYPE type, FMOD::Channel* ch)
        {
            lg("addFx");
            FMOD::DSP* dsp = nullptr;
            FMOD_RESULT res;
            lg("creating DSP...");
            res = system()->fmod()->createDSPByType(type, &dsp);
            if (res != FMOD_OK)
            {
                lg2("Can't create DSP", FMOD_ErrorString(res));
                return nullptr;
            }

            lg("adding DSP to channel...");
            //ch->setPaused(true);
            res = ch->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, dsp); // bug here, crash if the channel is playing.... Weird (only when set to TAIL).
            if (res != FMOD_OK)
            {
                lg2("Can't add DSP to channel", FMOD_ErrorString(res));
                return nullptr;
            }

            lg("Pushing dsp to _fxs...");
            vc::prepend(_fxs, std::move(std::unique_ptr<FMOD::DSP>(dsp)));
            lg("Pushing dsp to _fxs...");

            lg2("DSP ptr", dsp);
            return dsp;
        }

        FMOD::DSP* copyFx(FMOD::DSP* fx, mlSystem* sys, FMOD::Channel* ch)
        {
            FMOD::DSP* newFx = nullptr;
            FMOD_DSP_TYPE type;
            fx->getType(&type);
            sys->fmod()->createDSPByType(type, &newFx);
            ch->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, newFx);

            float prewet, postwet, dry;
            fx->getWetDryMix(&prewet, &postwet, &dry);
            newFx->setWetDryMix(prewet, postwet, dry);

            int num = 0;
            fx->getNumParameters(&num);
            for (int i=0; i<num; i++)
            {
                float val;
                fx->getParameterFloat(i, &val, nullptr, 0);
                newFx->setParameterFloat(i, val);
            }
            return newFx;
        }

        void removeFx(FMOD::DSP* dsp, FMOD::Channel* ch)
        {
            ch->removeDSP(dsp);
            for (auto it = _fxs.begin(); it != _fxs.end(); it++)
            {
                if ((*it).get() == dsp)
                {
                    (*it)->release();
                    _fxs.erase(it);
                    break;
                }
            }
        }

        std::vector<std::unique_ptr<FMOD::DSP>>& fxs(){return _fxs;}

        void Export(const std::string& path, mlSound* sound)
        {
            if (files::exists(path))
                files::remove(path);
            auto esys = mlSystem::createAndInit(path.c_str(),
                    FMOD_OUTPUTTYPE_WAVWRITER_NRT, 
                    512, 
                    FMOD_INIT_STREAM_FROM_UPDATE);

            std::unique_ptr<mlSound> ns(sound->copy(esys.get()));
            //TODO check the implemention live first.
//             for (auto& fx : _fxs) 
//             {
//                 auto nfx = copyFx(fx.get(), esys.get(), ns->ch());
//                 lg2("DSP copied", nfx);
//             }
// 
//             int dsps; 
//             ns->ch()->getNumDSPs(&dsps);
//             lg2("DSP number", dsps);

            lg(ns->freq());
            ns->play();
            auto total = ns->length();
            int i=0;
            while(!ns->finished())
            {
                if (i%100 == 0)
                {
                    float pgr = (float)ns->position()/(float)ns->length();
                    _events.emit("export-progress", pgr);
                }
                esys->fmod()->update();
                i++;
            }

            esys->release();
            _events.emit("export-finished");
        }
        ml::Events& events() { return _events; }
    }
}
