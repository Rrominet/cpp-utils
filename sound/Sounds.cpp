#define MINIAUDIO_IMPLEMENTATION
#include "./Sounds.h"
#include "./Sound.h"

namespace ml
{
    Sounds::Sounds() : _sounds(), _engine()
    {

    }

    Sounds::~Sounds()
    {
        if (_initialized)
            ma_engine_uninit(&_engine);
    }

    ml::Ret<> Sounds::_init()
    {
        if (_initialized)
            return ml::ret::success();

        if (ma_engine_init(nullptr, &_engine) != MA_SUCCESS)
            return ml::ret::fail("Failed to init miniaudio");

        _initialized = true;
        return ml::ret::success();
    }

    ml::Ret<ml::Sound*> Sounds::play(const std::string& path)
    {
        _init();
        auto s = std::make_unique<ml::Sound>(this, path);        
        auto res = s->init();
        if (!res.success)
            return ml::ret::fail<ml::Sound*>(res.message);
        s->play();
        _sounds.push_back(std::move(s));
        return ml::ret::success<ml::Sound*>(_sounds.back().get());
    }

    ml::Ret<ml::Sound*> Sounds::at(size_t index)
    {
        if (_sounds.size()<=index)        
            return ml::ret::fail<ml::Sound*>("Sounds::at - Index out of range : " + std::to_string(index));
        return ml::ret::success<ml::Sound*>(_sounds[index].get());
    }
}
