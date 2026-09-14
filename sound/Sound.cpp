#include "./Sound.h"
#include "../files.2/files.h"
#include "./Sounds.h"

namespace ml
{
    ml::Ret<> Sound::init()
    {
        if (_initialized)
            return ml::ret::success();

        if(!_manager)
            return ml::ret::fail("No sound manager setted for this sound.");

        if (!files::exists(_path))
            return ml::ret::fail("File not found : " + _path);

        if (files::type(_path) != files::SOUND)
            return ml::ret::fail("Not a sound file : " + _path);

        ma_result result = ma_sound_init_from_file(
                &_manager->engine(),
                _path.c_str(),
                0,          // flags
                nullptr,    // sound group
                nullptr,    // fence
                &_sound
                );

        if (result != MA_SUCCESS)
            return ml::ret::fail("Failed to init sound : " + _path);

        _initialized = true;
        return ml::ret::success();
    }

    Sound::~Sound()
    {
        if (_initialized)       
            ma_sound_uninit(&_sound);
    }

    void Sound::play()
    {
        ma_sound_start(&_sound);
    }

    void Sound::pause()
    {
        ma_sound_stop(&_sound);
    }

    void Sound::setVolume(float vol)
    {
        ma_sound_set_volume(&_sound, vol);        
    }

    float Sound::volume() const
    {
        return ma_sound_get_volume(&_sound);
    }

    bool Sound::playing() const
    {
        return ma_sound_is_playing(&_sound);
    }

    void Sound::toggle()
    {
        if (this->playing())
            this->pause();
        else
            this->play();
    }
}
