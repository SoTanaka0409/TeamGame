#include "SoundManager.h"

SoundManager::SoundManager()
{
}
SoundManager::~SoundManager()
{
    Clear();
}

void SoundManager::Load(const std::string &key, const std::string &path)
{
    if (sounds.find(key) == sounds.end())
    {
        int handle = LoadSoundMem(path.c_str());
        if (handle != -1)
        {
            sounds[key] = handle;
        }
    }
}

void SoundManager::Play(const std::string &key, bool loop)
{
    auto it = sounds.find(key);
    if (it != sounds.end())
    {
        PlaySoundMem(it->second,
                     DX_PLAYTYPE_BACK | (loop ? DX_PLAYTYPE_LOOP : 0));
    }
}

void SoundManager::Stop(const std::string &key)
{
    auto it = sounds.find(key);
    if (it != sounds.end())
    {
        StopSoundMem(it->second);
    }
}

void SoundManager::StopAll()
{
    InitSoundMem(); // DXLib specific to stop all sounds
}

void SoundManager::Clear()
{
    InitSoundMem();
    sounds.clear();
}
