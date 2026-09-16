#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "../Core/Vector2.h"

class SoundManager
{
  private:
    std::unordered_map<std::string, int> sounds;
    
    // 空間オーディオ用（3Dサウンド）のリスナー情報
    Vector2 listenerPos;
    Vector2 listenerDir;

    // 複製して再生したSEのハンドルリスト（終了時に削除するため）
    std::vector<int> playing3DSounds;

    SoundManager();
    ~SoundManager();
  public:
    static SoundManager& GetInstance()
    {
        static SoundManager instance;
        return instance;
    }

    // 毎フレーム呼ぶことで、再生が終わった複製音をメモリから解放する
    void Update();

    // リスナー（プレイヤー）の位置と向きをセット
    void SetListener(const Vector2& pos, const Vector2& dir);

    void Load(const std::string &key, const std::string &path);
    void Play(const std::string &key, bool loop = false);
    
    // 空間オーディオとして再生（VALORANT風）
    void Play3D(const std::string &key, const Vector2 &sourcePos, float maxDistance, float baseVolume = 1.0f);

    void Stop(const std::string &key);
    void StopAll();
    void Clear();
};
