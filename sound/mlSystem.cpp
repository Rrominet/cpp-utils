#include "debug.h"
#include "mlSystem.h"
#include "mlSound.h"
#include "files.2/files.h"
#include <memory>

using namespace std;

std::unique_ptr<mlSystem>  mlSystem::createAndInit(
        const char* filename, 
        FMOD_OUTPUTTYPE output, 
        const int &mxChannels, 
        FMOD_INITFLAGS flags, 
        void *extradriverdata)
{
    FMOD_RESULT res; 
    FMOD::System *sys = nullptr;
    res = FMOD::System_Create(&sys);

    if (res != FMOD_OK)
        lg2("Error : can't create fmod system", FMOD_ErrorString(res));
    else 
        lg("fmod system created.");

    //setting output 
    //in a file ? or in a driver to streaming live
    if (!filename)
        res = sys->setOutput(FMOD_OUTPUTTYPE_AUTODETECT);
    else 
        res = sys->setOutput(output);
    if (res!= FMOD_OK)
        lg2("can't set output to the system", FMOD_ErrorString(res));

    if (output == FMOD_OUTPUTTYPE_WAVWRITER)
        res = sys->init(512, FMOD_DEFAULT, (void*)filename);
    else if (output == FMOD_OUTPUTTYPE_WAVWRITER_NRT)
        res = sys->init(512, FMOD_INIT_STREAM_FROM_UPDATE, (void*)filename);
    else
        res = sys->init(512, FMOD_DEFAULT, nullptr);

    if (res != FMOD_OK)
        lg2("Error ! Can't init fmod system", FMOD_ErrorString(res));
    else 
        lg("fmod system initialized.");

    return std::make_unique<mlSystem>(sys);
}

FMOD::System* mlSystem::fmod()
{
    return _fmod;
}

mlSound* mlSystem::sound(const string &path,
        const MemoryRead &mem,
        FMOD_MODE mode, 
        FMOD_CREATESOUNDEXINFO *exinfo)
{
    for (auto&s : this->sounds())
    {
        if (s->path() == path)
            return s.get();
    }
    auto s = std::make_unique<mlSound>(path, this, mem, mode, exinfo);
    {
        std::lock_guard lk(_mtx);
        tmps.push(std::move(s));
    }
    return this->sounds().back().get();
}

mlSound* mlSystem::soundFromData(const char* data, FMOD_CREATESOUNDEXINFO *exinfo)
{
    this->sounds().push_back(std::make_unique<mlSound>(data, this,exinfo));
    return this->sounds().back().get();
}

void mlSystem::addSound(mlSound* s)
{
    std::unique_ptr<mlSound> tmp(s);
    {
        std::lock_guard lk(_mtx);
        this->sounds().push_back(std::move(tmp));
    }
}

void mlSystem::removeSound(mlSound* s)
{
    std::lock_guard lk(_mtx);
    for (auto it = this->sounds().vec.begin(); it != this->sounds().vec.end();)
    {
        if ((*it).get() == s)
            this->sounds().vec.erase(it);
        else 
            ++it;
    }
}

FMOD_RESULT mlSystem::release()
{
    this->clearSounds();
    lg("trying to release the system...");
    FMOD_RESULT res = _fmod->release();
    _fmod = nullptr;
    lg("Sys released");
    return res;
}

void mlSystem::clearSounds()
{
    std::lock_guard lk(_mtx);
    for (auto it = this->sounds().vec.begin(); it != this->sounds().vec.end();)
    {
        if (!(*it)->loading())
            this->sounds().vec.erase(it);
        else 
            ++it;
    }
}

unsigned int mlSystem::length()
{
    unsigned int totalLength = 0;   
    for (auto &s : this->sounds())
    {
        unsigned int _length; 
        s->_sound->getLength(&_length, FMOD_TIMEUNIT_PCM);
        totalLength += _length;
    }

    return totalLength;
}

void mlSystem::Export()
{
    if (files::exists(_out))
        files::remove(_out);
    for (auto &s : sounds())
    {
        s->play();
        while(!s->finished())
        {
            _fmod->update();
        }
    }
}
