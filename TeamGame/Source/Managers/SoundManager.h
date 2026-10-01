#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "../Core/Vector2.h"

/**
 * @brief サウンドを管理するシングルトンクラス
 * @details BGM・SEのロードと再生、および3D空間オーディオの制御を行う。
 */
class SoundManager
{
  private:
    std::unordered_map<std::string, int> sounds;
    
    // 空間オーディオ用（3Dサウンド）のリスナー情報
    Vector2 listenerPos;
    Vector2 listenerDir;

    // 複製して再生したSEのハンドルリスト（終了時に削除するため）
    std::vector<int> playing3DSounds;

    /**
     * @brief コンストラクタ
     */
    SoundManager();

    /**
     * @brief デストラクタ
     */
    ~SoundManager();
  public:
    /**
     * @brief インスタンスの取得
     * @return SoundManager& サウンドマネージャのシングルトンインスタンス
     */
    static SoundManager& GetInstance()
    {
        static SoundManager instance;
        return instance;
    }

    /**
     * @brief サウンド管理の更新処理
     * @details 毎フレーム呼ぶことで、再生が終わった複製音をメモリから解放する。
     */
    void Update();

    /**
     * @brief リスナーの設定
     * @param pos リスナー（プレイヤー）の位置
     * @param dir リスナーの向き
     * @details 3Dサウンド再生の基準となるリスナーの位置と向きをセットする。
     */
    void SetListener(const Vector2& pos, const Vector2& dir);

    /**
     * @brief サウンドの読み込み
     * @param key サウンドを識別するキー名
     * @param path サウンドファイルのパス
     * @details 指定したパスからサウンドを読み込み、キーに関連付けて保持する。
     */
    void Load(const std::string &key, const std::string &path);

    /**
     * @brief サウンドの再生
     * @param key 再生するサウンドのキー名
     * @param loop ループ再生するかどうか (デフォルト: false)
     * @details キーに関連付けられたサウンドを再生する。
     */
    void Play(const std::string &key, bool loop = false);
    
    /**
     * @brief 3Dサウンドの再生
     * @param key 再生するサウンドのキー名
     * @param sourcePos 音源の位置
     * @param maxDistance 音が届く最大距離
     * @param baseVolume 基準となる音量 (デフォルト: 1.0f)
     * @param sourceTeamId 音源の所属チームID (デフォルト: -1)
     * @return bool 再生に成功した場合はtrue
     * @details リスナーと音源の位置関係から音量とパンを計算し、空間オーディオとして再生する（VALORANT風）。
     */
    bool Play3D(const std::string &key, const Vector2 &sourcePos, float maxDistance, float baseVolume = 1.0f, int sourceTeamId = -1);

    /**
     * @brief サウンドの停止
     * @param key 停止するサウンドのキー名
     * @details 指定されたキーのサウンドの再生を停止する。
     */
    void Stop(const std::string &key);

    /**
     * @brief 全サウンドの停止
     * @details 再生中のすべてのサウンドを停止する。
     */
    void StopAll();

    /**
     * @brief リソースのクリア
     * @details 読み込まれたサウンドリソースを全て解放する。
     */
    void Clear();
};
