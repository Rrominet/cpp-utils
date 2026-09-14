#include "mlSound.h"
#include "debug.h"
#include <math.h>
#include <cstring>
#include "mlSystem.h"

mlSound::mlSound(const std::string &path, mlSystem* sys,
        int mem,
        FMOD_MODE mode, 
        FMOD_CREATESOUNDEXINFO *exinfo)
{
    lg("creating sound...");
    _loading = true;
    _path = path;
    _system = sys; 
    if (mem == mlSystem::UNBLOCKING)
    {
        res = _system->fmod()->createStream(path.c_str(), 
                mode, 
                exinfo, 
                &_sound);
    }

    else 
    {
        res = _system->fmod()->createSound(path.c_str(), 
                mode, 
                exinfo, 
                &_sound);
    }

    if (res != FMOD_OK)
        lg2("Error loading the sound", FMOD_ErrorString(res));
    else 
        lg("Sound created sucessfully");

    _loading = false;
    lg("mlSound constructor doned.");
}

mlSound::mlSound(const char* data, mlSystem* sys, FMOD_CREATESOUNDEXINFO *exinfo)
{
    lg("creating sound...");
    _system = sys; 

    res = _system->fmod()->createSound((const char*)data, 
            FMOD_OPENRAW | FMOD_OPENMEMORY_POINT, 
            exinfo, 
            &_sound);

    if (res != FMOD_OK)
        lg2("Error loading the sound", FMOD_ErrorString(res));
    else 
        lg("Sound created sucessfully");

    if (res != FMOD_OK)
        lg2("Can't put the sound on a the channel...", FMOD_ErrorString(res));
    else 
        lg2("initialized with this channel", _ch);
}

mlSound::~mlSound()
{
    if (_sound)
        _sound->release();

    for (auto &e : effects)
    {
        _ch->removeDSP(e);
        e->release();
    }
    effects.clear();
}

mlSound* mlSound::copy(mlSystem* sys)
{ 
    if (!sys)
        sys = this->system();
    mlSound* ns = new mlSound(this->path(), sys);
    ns->play(); // to setup the channel
    ns->pause();
    ns->setVolume(this->volume());
    ns->setFreq(this->freq());
    ns->_reversed = _reversed;
    if (_reversed)
        ns->setPosition(ns->length() - 1);
    else 
        ns->setPosition(0);
    return ns;
}

bool mlSound::play()
{
    lg("play !");
    if (!_ch)
    {
        res = _system->fmod()->playSound(sound(), nullptr, false, &_ch);
        if (res != FMOD_OK)
            throw std::runtime_error("Can't put the sound on a the channel..." + std::string(FMOD_ErrorString(res)));
        else 
            lg2("initialized with this channel", _ch);

        _baseFreq = this->freq();
    }

    if (!playing())
    {
        res = _ch->setPaused(false);
        setVolume(_volume);
    }
    else 
    {
        lg("already playing.");
        return false;
    }
    if (res != FMOD_OK)
    {
        lg2("can't play the sound", FMOD_ErrorString(res));
        return false;
    }
    _playing = true;
    return true;
}

bool mlSound::preloadChannel()
{
    if (!_ch)
    {
        res = _system->fmod()->playSound(sound(), nullptr, true, &_ch);
        if (res != FMOD_OK)
            throw std::runtime_error("Can't put the sound on a the channel..." + std::string(FMOD_ErrorString(res)));
        else 
            lg2("initialized with this channel", _ch);

        _baseFreq = this->freq();
        if (res != FMOD_OK)
        {
            lg2("can't play the sound", FMOD_ErrorString(res));
            return false;
        }
        else 
            return true;
    }
    else 
        return false;
}

bool mlSound::pause()
{
    if (!playing())
    {
        lg("The sound is already paused !");
        return false;
    }
    else 
    {
        if (!_ch)
            return false;
        res = _ch->setPaused(true);
        if (res == FMOD_OK)
        {
            _playing = false;
            return true;
        }
        else 
        {
            lg2("Can't pause the sound", FMOD_ErrorString(res));
            return false ;
        }
    }

    return false ;
}

bool mlSound::playing()
{
    return _playing;
}

bool mlSound::restart()
{
    bool r = setPosition(0);
    if (!r)
        return r;
    _system->fmod()->playSound(sound(), nullptr, true, &_ch);
    return play();
}

bool mlSound::toggle()
{
    if (!playing())
        return play();
    else 
        return pause();
}

bool mlSound::mute()
{
    if (!_ch)
        return false;
    res = _ch->setMute(true);
    if (res == FMOD_OK)
        return true;
    lg2("Can't mute the sound.", FMOD_ErrorString(res));
    return false;
}

bool mlSound::unmute()
{
    if (!_ch)
        return false;
    res = _ch->setMute(false);
    if (res == FMOD_OK)
        return true;

    lg2("Can't unmute the sound.", FMOD_ErrorString(res));
    return false;
}

bool mlSound::toggleMute()
{
    if (this->muted())
        return this->unmute();
    else 
        return this->mute();
}

bool mlSound::setPan(const float &pan)
{
    if (!_ch)
        return false;
    res = _ch->setPan(pan) ;
    if (res == FMOD_OK)
        return true;

    lg2("Can't pan the sound.", FMOD_ErrorString(res));
    return false;
}

bool mlSound::setPitch(const float &p)
{
    if (p<=0)
        return false;
    if (!_ch)
        return false;
    res = _ch->setPitch(p) ;
    lg2("Current pitch", pitch());
    if (res == FMOD_OK)
        return true;

    lg2("Can't pan the sound.", FMOD_ErrorString(res));
    return false;
}

float mlSound::pitch()
{
    float pitched = 0.0;     
    if (!_ch)
        return 0.0;
    res = _ch->getPitch(&pitched);

    return pitched;
}

bool mlSound::muted()
{
    bool muted = false;     
    if (!_ch)
        return false;
    res = _ch->getMute(&muted);

    return muted;
}

bool mlSound::setVolume(float vol)
{
    _volume = vol;
    if (!_ch)
        return false;
    lg2("The intern volume is", _volume);
    if (vol <0.0)
        vol = 0.0;
    res = _ch->setVolume(_volume) ;
    lg2("the volume is now", volume());
    if (res == FMOD_OK)
        return true;

    lg2("Can't change volume of the sound.", FMOD_ErrorString(res));
    return false;
}

bool mlSound::setVolumeDb(float vol)
{
    float linear = pow(10, vol/20); 
    return setVolume(linear);
}

float mlSound::volume()
{
    float v = 0.0;     
    if (!_ch)
        return v;
    res = _ch->getVolume(&v);

    return v;
}

float mlSound::volumeDb()
{
    return log(volume()) * 20;
}

bool mlSound::reverse()
{
    if (!_ch)
        return false;
    bool res = setFreq(-freq());
    if (freq()<0)
    {
        _reversed = true;
        _ch->setPosition(length() - 1, FMOD_TIMEUNIT_PCM);
        lg2("Sound has been reversed, this is new position", position());
    }
    else 
    {
        _reversed = false;
        _ch->setPosition(0, FMOD_TIMEUNIT_PCM);
    }
    return res;
}

float mlSound::freq()
{
    float f = 0.0;     
    if (!_ch)
        return 0.0;
    res = _ch->getFrequency(&f);

    return f;
}

bool mlSound::setFreq(float f)
{
    if (!_ch)
        return false;
    res = _ch->setFrequency(f) ;
    lg2("the frequency is now", freq());

    if (res == FMOD_OK)
    {
        _events.emit("freq-changed", this->freq());
        return true;
    }

    lg2("Can't change frequency of the sound.", FMOD_ErrorString(res));
    return false;
}

unsigned int mlSound::length()
{
    unsigned int l;
    res =  _sound->getLength(&l, FMOD_TIMEUNIT_PCM);
    if (res != FMOD_OK)
        lg2("Can't get sound length", FMOD_ErrorString(res));

    return l;
}

unsigned int mlSound::totalLength()
{
    return this->length();
}

unsigned int mlSound::msLength()
{
    unsigned int l;
    res =  _sound->getLength(&l, FMOD_TIMEUNIT_MS);
    if (res != FMOD_OK)
        lg2("Can't get sound length", FMOD_ErrorString(res));

    return l;
}

unsigned int mlSound::byteLength()
{
    auto f = this->format();
    return this->length() * f.bits/8 * f.channels;
}

unsigned int mlSound::position()
{
    if (!_ch)
        return 0;
    unsigned int p;    
    res = _ch->getPosition(&p, FMOD_TIMEUNIT_PCM); 
    if (res != FMOD_OK)
        lg2("Can't get sound position", FMOD_ErrorString(res));
    return p;
}

unsigned int mlSound::bytePosition()
{
    auto f = this->format();
    return this->position() * (f.bits/8) * f.channels;
}

unsigned int mlSound::msposition()
{
    if (!_ch)
        return 0;
    unsigned int p;    
    res = _ch->getPosition(&p, FMOD_TIMEUNIT_MS); 
    if (res != FMOD_OK)
        lg2("Can't get sound position", FMOD_ErrorString(res));
    return p;
}

float mlSound::positionRatio()
{
    return (this->position()*1.0)/(this->length()*1.0);
}

bool mlSound::setPosition(const unsigned int& p)
{ 
    if (!_ch)
        return false;
    res = _ch->setPosition(p, FMOD_TIMEUNIT_PCM); 
    if (res != FMOD_OK)
    {
        lg2("Can't set sound position", FMOD_ErrorString(res));
        return false;
    }
    return true;
}

bool mlSound::setPositionRatio(const float &val)
{
    return this->setPosition(val*this->length());
}

bool mlSound::setMsposition(const unsigned int& p)
{
    if (!_ch)
        return 0;
    res = _ch->setPosition(p, FMOD_TIMEUNIT_MS); 
    if (res != FMOD_OK)
    {
        lg2("Can't set sound position", FMOD_ErrorString(res));
        return false;
    }
    return true;
}

bool mlSound::movePrevious(const unsigned int& p)
{
    return this->setPosition(this->position() - p);
}
bool mlSound::moveMsPrevious(const unsigned int& p)
{
    return this->setMsposition(this->msposition() - p);
}

bool mlSound::moveNext(const unsigned int& p)
{
    return this->setPosition(this->position() + p);
}
bool mlSound::moveMsNext(const unsigned int& p)
{
    return this->setMsposition(this->msposition() + p);
}

bool mlSound::finished()
{
    bool res = false;
    if (!_reversed)
        res = (this->position() >= this->length() - 1);
    else
        res = (this->position() > this->length() - 1);
    return res ;
}

FMOD::DSP* mlSound::newEffect(FMOD_DSP_TYPE type)
{
    FMOD::DSP* dsp = nullptr; 
    res = _system->fmod()->createDSPByType(type, &dsp);
    if (res != FMOD_OK)
        lg2("Dsp creation failed", FMOD_ErrorString(res));
    effects.push_back(dsp);
    res = _ch->addDSP(effects.size() - 1, dsp);
    if (res != FMOD_OK)
        lg2("Cant attach FMOD::DSP to channel", FMOD_ErrorString(res));
    return dsp;
}

bool mlSound::setSpeed(float f)
{
    _speed = f;
    _events.emit("speed-changed", f);
    return this->setFreq(_baseFreq * _speed);
}

SoundData mlSound::format(FMOD::Sound* s)
{
    SoundData meta; 
    s->getFormat(
            &meta.type,
            &meta.format,
            &meta.channels,
            &meta.bits);

    return meta;
}

unsigned int mlSound::length(FMOD::Sound* s)
{
    unsigned int length = 0;
    s->getLength(
            &length,
            FMOD_TIMEUNIT_MS);

    return length;
}

SoundData* mlSound::data(unsigned int offset, 
        unsigned int length)
{
    auto sd = new SoundData;
    if (length == 0)
        length = this->byteLength() - offset;

    void *d1;
    void *d2;
    unsigned int l1 = 0;
    unsigned int l2 = 0;

    this->sound()->lock(offset, length, 
            &d1, &d2, &l1, &l2);

    sd->length = l1 + l2;
    if (l2 == 0)
    {
        sd->data = malloc(l1);
        sd->data = memcpy(sd->data, d1, l1);
    }
    else 
    {
        lg("Error : data is bigger than the sampling buffer...");
        // here i need to implement the double buffer appending but i'ts not that easy...
    }

    this->sound()->getFormat(&sd->type, &sd->format, &sd->channels, &sd->bits);
    this->sound()->unlock(d1, d2, l1, l2);

    return sd;
}

SoundData::~SoundData()
{
    free(this->data);
}

std::vector<float> SoundData::dataPart(long long unsigned begin, long long unsigned end, unsigned int step)
{
    if (end == 0)
        end = length - 1;
    return this->dataPart2(begin, end - begin, step);
}

std::vector<float> SoundData::dataPart2(long long unsigned begin, long long unsigned offset, unsigned int step)
{
    if (begin<0)
        begin = 0;
    if (begin + offset>= this->length)
        offset = this->length - begin -1;
    std::vector<float> _r;
    auto f = [&](float sample){_r.push_back(sample);};
    this->processOnEverySamples(f, offset, begin, step); // it seams to BUG here ! :o
    return _r;
}

void SoundData::processOnOneSample(const boost::function <void (float)>& f, unsigned long long offset)
{
    float d = 0.0;
    char* data = this->getData() + offset;
    if (this->bits == 32 && this->format == FMOD_SOUND_FORMAT_PCMFLOAT) 
        d = *(float*)data;
    else 
    {
        if (this->bits == 8)
            d = ((float)(*data) - 128) / 128;
        else if (this->bits == 16)
        {
            short td = this->getData()[offset];
            td |= (this->getData()[offset + 1] << 8);
            d = (float(td)/32768);
        }
        else if (this->bits == 24)
        {
            int di = this->getData()[offset];
            di |= (this->getData()[offset + 1] << 8);
            di |= (this->getData()[offset + 2] << 16);

            // Check if the 24th bit is set (negative number)
            if (di & 0x00800000)
                di |= 0xFF000000; // Extend the sign bit to the 32-bit integer
            else
                di &= 0x00FFFFFF; // Ensure upper 8 bits are zero for positive numbers

            d = di / 8388608.0f; // Divide by 2^23 to normalize
        }

        else if (this->bits == 32)
        {
            int td = *(int*)data; // 4294967296
            d = (float(td)/2147483647);
        }
    }

    f(d);
}

unsigned int mlSound::byteSize(unsigned int ms)
{
    float freqMs = this->freq()/1000;
    int units = freqMs * ms;
    auto f = this->format();

    return units * (f.bits/8) * f.channels;
}

void SoundData::processOnEverySamples(const boost::function <void (float)>& f, unsigned long long length, unsigned long long offset, unsigned int step, unsigned int* currentStep)
{
    int increment = this->bits/8;
    auto mod = offset%increment;
    if (mod!=0)
        offset -= mod;

    if (length == 0)
        length = this->length;
    if (offset + length > this->length)
    {
        lg("error : the offset + length is superior to the hole sound length !");
        lg2("increment", increment);
        lg2("offset", offset);
        lg2("length", length);
        lg2("offet + length", offset + length);
        lg2("Sound length", this->length);
        return;
    }

    if (currentStep)
        *currentStep = 0;
    for (int i=offset; i< offset + length; i+= (increment * step))
    {
        this->processOnOneSample(f, i);
        if (currentStep)
            (*currentStep)++;
    }
}

float mlSound::samplesNumber(int ms)
{
    if (ms == -1)
        ms = this->msLength();
    auto samplePerMs = this->freq()/1000;
    return samplePerMs*ms;
}

void mlSound::loop()
{
    if (this->looped())
        return;
    _ch->setMode(FMOD_LOOP_NORMAL);
}

void mlSound::unloop()
{
    if (!this->looped())
        return;
    _ch->setMode(FMOD_LOOP_OFF);
}

bool mlSound::looped()
{
    FMOD_MODE mode;
    res = _ch->getMode(&mode);
    if (res != FMOD_OK)
        return false;
    return mode == FMOD_LOOP_NORMAL;
}

long mlSound::memory()
{
    return this->byteLength();
}
