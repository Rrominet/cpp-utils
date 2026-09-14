#include "./Sound.h"
#include "../files.2/files.h"
#include "./Sounds.h"

namespace ml
{
    ml::Ret<> Sound::init(LoadType type)
    {
        if (_initialized)
            return ml::ret::success();

        if(!_manager)
            return ml::ret::fail("No sound manager setted for this sound.");

        if (!files::exists(_path))
            return ml::ret::fail("File not found : " + _path);

        if (files::type(_path) != files::SOUND)
            return ml::ret::fail("Not a sound file : " + _path);

        auto flag = 0;
        if (type == SYNC_DECODE)
            flag = MA_SOUND_FLAG_DECODE;
        else if (type == SYNC_NO_DECODE)
            flag = MA_SOUND_FLAG_STREAM;
        else if (type == ASYNC)
            flag = MA_SOUND_FLAG_ASYNC;

        ma_result result = ma_sound_init_from_file(
                &_manager->engine(),
                _path.c_str(),
                flag,
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

    double Sound::mspos()
    {
        float seconds = 0.0f;
        ma_sound_get_cursor_in_seconds(&_sound, &seconds);
        return static_cast<double>(seconds) * 1000.0;
    }

    double Sound::mslength() const
    {
        float seconds = 0.0f;
        ma_sound_get_length_in_seconds(&_sound, &seconds);
        return static_cast<double>(seconds) * 1000.0;
    }

    void Sound::setMsPos(double pos)
    {
        ma_sound_seek_to_second(&_sound, static_cast<float>(pos / 1000.0));
    }

    void Sound::setPos(float ratio)
    {
        auto len = this->mslength();	
        this->setMsPos(len * ratio);
    }

    void Sound::setPan(float pan)
    {
        ma_sound_set_pan(&_sound, pan);
    }

    float Sound::pan() const
    {
        return ma_sound_get_pan(&_sound);
    }

    void Sound::setPitch(float pitch)
    {
        ma_sound_set_pitch(&_sound, pitch);
    }

    float Sound::pitch() const
    {
        return ma_sound_get_pitch(&_sound);
    }

    long long unsigned Sound::pcmLength() const
    {
        ma_uint64 length = 0;
        ma_sound_get_length_in_pcm_frames(&_sound, &length);
        return static_cast<long long unsigned>(length);
    }

    long long unsigned Sound::pcmPos() const
    {
        ma_uint64 position = 0;
        ma_sound_get_cursor_in_pcm_frames(&_sound, &position);
        return static_cast<long long unsigned>(position);
    }

    void Sound::setPcmPos(long long unsigned pos)
    {
        ma_sound_seek_to_pcm_frame(&_sound, static_cast<ma_uint64>(pos));
    }

    void Sound::setLooping(bool looping)
    {
        if (looping)
            ma_sound_set_looping(&_sound, MA_TRUE);
        else
            ma_sound_set_looping(&_sound, MA_FALSE);
    }

    bool Sound::looping() const
    {
        return ma_sound_is_looping(&_sound);
    }
}
