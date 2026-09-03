#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>

class SoundManager
{
  private:
    std::unordered_map<std::string, int> sounds;

  public:
    SoundManager();
    ~SoundManager();

    void Load(const std::string &key, const std::string &path);
    void Play(const std::string &key, bool loop = false);
    void Stop(const std::string &key);
    void StopAll();
    void Clear();
};
