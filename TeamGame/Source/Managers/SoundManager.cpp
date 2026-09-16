#include "SoundManager.h"
#include <cmath>
#include <algorithm>

SoundManager::SoundManager() : listenerPos(0, 0), listenerDir(0, -1)
{
}
SoundManager::~SoundManager()
{
    Clear();
}

void SoundManager::Update()
{
    // 再生中の複製サウンドを監視し、再生が終了したものはメモリから削除する
    for (auto it = playing3DSounds.begin(); it != playing3DSounds.end(); )
    {
        // 0なら再生終了
        if (CheckSoundMem(*it) == 0)
        {
            DeleteSoundMem(*it);
            it = playing3DSounds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SoundManager::SetListener(const Vector2& pos, const Vector2& dir)
{
    listenerPos = pos;
    // 向きベクトルがゼロでなければ正規化する
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length > 0.0001f) {
        listenerDir = Vector2(dir.x / length, dir.y / length);
    }
}

void SoundManager::Play3D(const std::string &key, const Vector2 &sourcePos, float maxDistance, float baseVolume)
{
    auto it = sounds.find(key);
    if (it == sounds.end()) return;

    // 1. 距離の計算
    float dx = sourcePos.x - listenerPos.x;
    float dy = sourcePos.y - listenerPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // 最大距離を超えていたら鳴らさない
    if (distance > maxDistance) return;

    // 2. 音量の計算 (距離が近いほど大きく、遠いほど小さく)
    float volumeRatio = 1.0f - (distance / maxDistance);
    int volume = static_cast<int>(255.0f * volumeRatio * baseVolume);
    volume = std::max(0, std::min(255, volume));

    // 3. パン（左右）の計算
    // 音源へのベクトルを正規化
    float pan = 0;
    if (distance > 0.0001f) {
        float dirToSourceX = dx / distance;
        float dirToSourceY = dy / distance;
        
        // プレイヤーの向きベクトルとの外積計算で左右を判定
        // Z上向きの2D座標系として： (listenerDir.x * dirToSourceY) - (listenerDir.y * dirToSourceX)
        float crossProduct = (listenerDir.x * dirToSourceY) - (listenerDir.y * dirToSourceX);
        
        // crossProductは -1.0(左) ～ 1.0(右) の値になるため、DxLibの -255 ～ 255 に変換
        pan = crossProduct * 255.0f; 
    }

    // 4. 音データの複製と再生
    int dupHandle = DuplicateSoundMem(it->second);
    if (dupHandle != -1) {
        ChangeVolumeSoundMem(volume, dupHandle);
        ChangePanSoundMem(static_cast<int>(pan), dupHandle);
        PlaySoundMem(dupHandle, DX_PLAYTYPE_BACK);
        
        // 終了後に削除するためリストに追加
        playing3DSounds.push_back(dupHandle);
    }
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
