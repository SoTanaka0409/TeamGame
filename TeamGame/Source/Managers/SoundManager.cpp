#define NOMINMAX
#include "SoundManager.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ObjectManager.h"
#include "../Characters/Enemy.h"
#include <cmath>
#include <algorithm>

SoundManager::SoundManager() : listenerPos(0, 0), listenerDir(0, -1)
{
}
SoundManager::~SoundManager()
{
    // Clear();
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

bool SoundManager::Play3D(const std::string &key, const Vector2 &sourcePos, float maxDistance, float baseVolume, int sourceTeamId)
{
    auto it = sounds.find(key);
    if (it == sounds.end()) return false;

    float dx = sourcePos.x - listenerPos.x;
    float dy = sourcePos.y - listenerPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    bool playForPlayer = (distance <= maxDistance);
    if (playForPlayer) {
        float volumeRatio = 1.0f - (distance / maxDistance);
        int volume = static_cast<int>(255.0f * volumeRatio * baseVolume);
        volume = std::max(0, std::min(255, volume));

        float pan = 0;
        if (distance > 0.0001f) {
            float dirToSourceX = dx / distance;
            float dirToSourceY = dy / distance;
            float crossProduct = (listenerDir.x * dirToSourceY) - (listenerDir.y * dirToSourceX);
            pan = crossProduct * 255.0f; 
        }

        int dupHandle = DuplicateSoundMem(it->second);
        if (dupHandle != -1) {
            ChangeVolumeSoundMem(volume, dupHandle);
            ChangePanSoundMem(static_cast<int>(pan), dupHandle);
            PlaySoundMem(dupHandle, DX_PLAYTYPE_BACK);
            playing3DSounds.push_back(dupHandle);
        }
    }

    bool heardByAnyone = false;
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            Enemy *enemy = dynamic_cast<Enemy *>(obj);
            if (enemy && enemy->IsActive() && enemy->teamId != sourceTeamId)
            {
                float dx2 = enemy->GetPosition().x - sourcePos.x;
                float dy2 = enemy->GetPosition().y - sourcePos.y;
                float dist2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
                if (dist2 < maxDistance) {
                    enemy->OnHearGunshot(sourcePos, maxDistance);
                    heardByAnyone = true;
                }
            }
        }
    }
    return heardByAnyone;
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
