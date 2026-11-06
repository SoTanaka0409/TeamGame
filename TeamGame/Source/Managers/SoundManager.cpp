#define NOMINMAX
#include "SoundManager.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ObjectManager.h"
#include "../Characters/Enemy.h"
#include <cmath>
#include <algorithm>
#include <vector>

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
    if (it == sounds.end())
    {
        if (key == "grass_walk") { Load("grass_walk", "SE/足音/grass_walk.mp3"); it = sounds.find(key); }
        else if (key == "dirt_walk") { Load("dirt_walk", "SE/足音/dirt_walk.mp3"); it = sounds.find(key); }
        else if (key == "handgun_shot") { Load("handgun_shot", "SE/銃/gun_handgun.mp3"); it = sounds.find(key); }
        else if (key == "smg_shot") { Load("smg_shot", "SE/銃/gun_smg.mp3"); it = sounds.find(key); }
        else if (key == "shotgun_shot") { Load("shotgun_shot", "SE/銃/gun_shotgun.mp3"); it = sounds.find(key); }
        else if (key == "sniper_shot") { Load("sniper_shot", "SE/銃/gun_sniper.mp3"); it = sounds.find(key); }
        else if (key == "gunshot") { Load("gunshot", "SE/銃/gun_assault.mp3"); it = sounds.find(key); }
        else if (key == "reload") { Load("reload", "SE/銃/gun_reload.mp3"); it = sounds.find(key); }
        else if (key == "skill_activate") { Load("skill_activate", "SE/システム/se_skill.mp3"); it = sounds.find(key); }
        if (it == sounds.end()) return false;
    }

    float dx = sourcePos.x - listenerPos.x;
    float dy = sourcePos.y - listenerPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    bool playForPlayer = (distance <= maxDistance);
    if (playForPlayer) {
        float volumeRatio = 1.0f - (distance / maxDistance);
        int volume = static_cast<int>(255.0f * volumeRatio * baseVolume);
        volume = std::max(0, std::min(255, volume));

        float pan = 0.0f;
        // リスナー自身（プレイヤー）に近い場合は中央ステレオ再生
        if (distance > 30.0f) {
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

    // 銃声のみ敵AIに探知させる（足音などの移動音は除外）
    bool heardByAnyone = false;
    if (key == "gunshot" || key == "sniper_shot" || key == "handgun_shot" || key == "shotgun_shot" || key == "smg_shot")
    {
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
    }
    return heardByAnyone;
}

void SoundManager::Load(const std::string &key, const std::string &path)
{
    if (sounds.find(key) == sounds.end())
    {
        int handle = LoadSoundMem(path.c_str());
        if (handle == -1)
        {
            std::vector<std::string> fallbacks = {
                "Resource/" + path,
                "Resource/BGM/" + path,
                "Resource/SE/" + path,
                "Resource/SE/銃/" + path,
                "Resource/SE/足音/" + path,
                "Resource/SE/環境音/" + path,
                "Resource/SE/システム/" + path,
                "Resource/AUDIO/" + path,
                "TeamGame/Resource/" + path,
                "TeamGame/Resource/BGM/" + path,
                "TeamGame/Resource/SE/" + path,
                "TeamGame/Resource/SE/銃/" + path,
                "TeamGame/Resource/SE/足音/" + path,
                "TeamGame/Resource/SE/環境音/" + path,
                "TeamGame/Resource/SE/システム/" + path,
                "TeamGame/Resource/AUDIO/" + path,
                "../Resource/" + path,
                "../../Resource/" + path,
                "C:/Users/student/Desktop/SE・BGM/SE/" + path,
                "C:/Users/student/Desktop/SE・BGM/AUDIO/" + path,
                "C:/Users/student/Desktop/SE・BGM/" + path
            };
            if (key == "grass_walk")
            {
                fallbacks.push_back("Resource/SE/足音/grass_walk.mp3");
                fallbacks.push_back("Resource/SE/足音/草の上を歩く音風Ⅰ.mp3");
                fallbacks.push_back("Resource/SE/grass_walk.mp3");
                fallbacks.push_back("Resource/grass_walk.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/足音/grass_walk.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/足音/草の上を歩く音風Ⅰ.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/grass_walk.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/AUDIO/草の上を歩く音風Ⅰ.mp3");
            }
            else if (key == "dirt_walk")
            {
                fallbacks.push_back("Resource/SE/足音/dirt_walk.mp3");
                fallbacks.push_back("Resource/SE/足音/土の上を歩く音風.mp3");
                fallbacks.push_back("Resource/SE/dirt_walk.mp3");
                fallbacks.push_back("Resource/dirt_walk.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/足音/dirt_walk.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/足音/土の上を歩く音風.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/dirt_walk.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/AUDIO/土の上を歩く音風.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/環境音/土の上を歩く.mp3");
            }
            else if (key == "smg_shot")
            {
                fallbacks.push_back("Resource/SE/銃/gun_smg.mp3");
                fallbacks.push_back("Resource/SE/銃/サブマシンガン.mp3");
                fallbacks.push_back("Resource/SE/銃/サブマシンガン単発.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/gun_smg.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/サブマシンガン.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/サブマシンガン単発.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/サブマシンガン.mp3");
            }
            else if (key == "handgun_shot")
            {
                fallbacks.push_back("Resource/SE/銃/gun_handgun.mp3");
                fallbacks.push_back("Resource/SE/銃/拳銃を撃つ.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/gun_handgun.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/拳銃を撃つ.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/銃/拳銃を撃つ.mp3");
            }
            else if (key == "shotgun_shot")
            {
                fallbacks.push_back("Resource/SE/銃/gun_shotgun.mp3");
                fallbacks.push_back("Resource/SE/銃/ショットガン発射.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/gun_shotgun.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/ショットガン発射.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/銃/ショットガン発射.mp3");
            }
            else if (key == "sniper_shot")
            {
                fallbacks.push_back("Resource/SE/銃/gun_sniper.mp3");
                fallbacks.push_back("Resource/SE/銃/狙撃銃発射1.mp3");
                fallbacks.push_back("Resource/SE/銃/狙撃銃発射2.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/gun_sniper.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/狙撃銃発射1.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/銃/狙撃銃発射1.mp3");
            }
            else if (key == "gunshot")
            {
                fallbacks.push_back("Resource/SE/銃/gun_assault.mp3");
                fallbacks.push_back("Resource/SE/銃/アサルト単発音.mp3");
                fallbacks.push_back("Resource/SE/銃/gun_handgun.mp3");
                fallbacks.push_back("Resource/SE/銃/拳銃を撃つ.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/gun_assault.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/アサルト単発音.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/銃/アサルト単発音.mp3");
            }
            else if (key == "reload")
            {
                fallbacks.push_back("Resource/SE/銃/gun_reload.mp3");
                fallbacks.push_back("Resource/SE/銃/銃のリロード.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/gun_reload.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/銃/銃のリロード.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/銃/銃のリロード.mp3");
            }
            else if (key == "title_bgm")
            {
                fallbacks.push_back("Resource/BGM/bgm_title.mp3");
                fallbacks.push_back("Resource/BGM/タイトル.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/bgm_title.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/タイトル.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/BGM/タイトル.mp3");
            }
            else if (key == "battle_bgm")
            {
                fallbacks.push_back("Resource/BGM/bgm_battle.mp3");
                fallbacks.push_back("Resource/BGM/戦闘.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/bgm_battle.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/戦闘.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/BGM/戦闘.mp3");
            }
            else if (key == "clear_bgm")
            {
                fallbacks.push_back("Resource/BGM/bgm_clear.mp3");
                fallbacks.push_back("Resource/BGM/クリア.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/bgm_clear.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/クリア.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/BGM/クリア.mp3");
            }
            else if (key == "defeat_bgm")
            {
                fallbacks.push_back("Resource/BGM/bgm_defeat.mp3");
                fallbacks.push_back("Resource/BGM/敗北音.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/bgm_defeat.mp3");
                fallbacks.push_back("TeamGame/Resource/BGM/敗北音.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/BGM/敗北音.mp3");
            }
            else if (key == "se_tap")
            {
                fallbacks.push_back("Resource/SE/システム/se_tap.mp3");
                fallbacks.push_back("Resource/SE/システム/タップ音.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/システム/se_tap.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/システム/タップ音.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/SE/タップ音.mp3");
            }
            else if (key == "skill_activate")
            {
                fallbacks.push_back("Resource/SE/システム/se_skill.mp3");
                fallbacks.push_back("Resource/SE/システム/スキル発動.mp3");
                fallbacks.push_back("Resource/SE/se_skill.mp3");
                fallbacks.push_back("Resource/SE/スキル発動.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/システム/se_skill.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/システム/スキル発動.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/se_skill.mp3");
                fallbacks.push_back("TeamGame/Resource/SE/スキル発動.mp3");
                fallbacks.push_back("C:/Users/student/Desktop/SE・BGM/スキル発動.mp3");
            }
            for (const auto& fb : fallbacks)
            {
                handle = LoadSoundMem(fb.c_str());
                if (handle != -1) break;
            }
        }
        if (handle != -1)
        {
            if (key == "reload")
            {
                ChangeVolumeSoundMem(145, handle); // リロード音を抑えめに設定 (約57%)
            }
            sounds[key] = handle;
        }
    }
}

void SoundManager::Play(const std::string &key, bool loop)
{
    auto it = sounds.find(key);
    if (it == sounds.end())
    {
        if (key == "reload") { Load("reload", "SE/銃/gun_reload.mp3"); it = sounds.find(key); }
        else if (key == "gunshot") { Load("gunshot", "SE/銃/gun_assault.mp3"); it = sounds.find(key); }
        else if (key == "handgun_shot") { Load("handgun_shot", "SE/銃/gun_handgun.mp3"); it = sounds.find(key); }
        else if (key == "smg_shot") { Load("smg_shot", "SE/銃/gun_smg.mp3"); it = sounds.find(key); }
        else if (key == "shotgun_shot") { Load("shotgun_shot", "SE/銃/gun_shotgun.mp3"); it = sounds.find(key); }
        else if (key == "sniper_shot") { Load("sniper_shot", "SE/銃/gun_sniper.mp3"); it = sounds.find(key); }
        else if (key == "se_tap") { Load("se_tap", "SE/システム/se_tap.mp3"); it = sounds.find(key); }
        else if (key == "skill_activate") { Load("skill_activate", "SE/システム/se_skill.mp3"); it = sounds.find(key); }
    }
    if (it != sounds.end())
    {
        PlaySoundMem(it->second,
                     DX_PLAYTYPE_BACK | (loop ? DX_PLAYTYPE_LOOP : 0));
    }
}

void SoundManager::PlayTapSE(float volume)
{
    auto it = sounds.find("se_tap");
    if (it == sounds.end())
    {
        Load("se_tap", "SE/システム/se_tap.mp3");
        it = sounds.find("se_tap");
    }
    if (it != sounds.end())
    {
        int vol = static_cast<int>(255.0f * std::max(0.0f, std::min(1.0f, volume)));
        ChangeVolumeSoundMem(vol, it->second);
        PlaySoundMem(it->second, DX_PLAYTYPE_BACK);
    }
}

void SoundManager::PlaySkillSE(float volume)
{
    auto it = sounds.find("skill_activate");
    if (it == sounds.end())
    {
        Load("skill_activate", "SE/システム/se_skill.mp3");
        it = sounds.find("skill_activate");
    }
    if (it != sounds.end())
    {
        int vol = static_cast<int>(255.0f * std::max(0.0f, std::min(1.0f, volume)));
        ChangeVolumeSoundMem(vol, it->second);
        PlaySoundMem(it->second, DX_PLAYTYPE_BACK);
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

void SoundManager::PlayBGM(const std::string &key, float volume)
{
    int vol = static_cast<int>(255.0f * std::max(0.0f, std::min(1.0f, volume)));

    // 同じBGMが既に再生中の場合は、曲を最初に戻さず音量調整のみ行いそのまま再生継続
    if (currentBGMKey == key)
    {
        auto it = sounds.find(key);
        if (it != sounds.end())
        {
            if (CheckSoundMem(it->second) == 0)
            {
                PlaySoundMem(it->second, DX_PLAYTYPE_LOOP);
            }
            ChangeVolumeSoundMem(vol, it->second);
        }
        return;
    }

    // 別のBGMが流れているなら停止
    if (!currentBGMKey.empty())
    {
        auto itOld = sounds.find(currentBGMKey);
        if (itOld != sounds.end())
        {
            StopSoundMem(itOld->second);
        }
    }

    currentBGMKey = key;
    auto it = sounds.find(key);
    if (it == sounds.end())
    {
        if (key == "title_bgm") { Load("title_bgm", "BGM/bgm_title.mp3"); it = sounds.find(key); }
        else if (key == "battle_bgm") { Load("battle_bgm", "BGM/bgm_battle.mp3"); it = sounds.find(key); }
        else if (key == "clear_bgm") { Load("clear_bgm", "BGM/bgm_clear.mp3"); it = sounds.find(key); }
        else if (key == "defeat_bgm") { Load("defeat_bgm", "BGM/bgm_defeat.mp3"); it = sounds.find(key); }
    }

    if (it != sounds.end())
    {
        ChangeVolumeSoundMem(vol, it->second);
        PlaySoundMem(it->second, DX_PLAYTYPE_LOOP);
    }
}

void SoundManager::StopBGM()
{
    if (!currentBGMKey.empty())
    {
        auto it = sounds.find(currentBGMKey);
        if (it != sounds.end())
        {
            StopSoundMem(it->second);
        }
        currentBGMKey = "";
    }
}

void SoundManager::SetBGMVolume(float volume)
{
    if (!currentBGMKey.empty())
    {
        auto it = sounds.find(currentBGMKey);
        if (it != sounds.end())
        {
            int vol = static_cast<int>(255.0f * std::max(0.0f, std::min(1.0f, volume)));
            ChangeVolumeSoundMem(vol, it->second);
        }
    }
}

void SoundManager::StopAll()
{
    currentBGMKey = "";
    InitSoundMem(); // DXLib specific to stop all sounds
}

void SoundManager::Clear()
{
    InitSoundMem();
    sounds.clear();
}
