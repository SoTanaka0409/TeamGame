#pragma once
#include "../Core/Vector2.h"
#include <vector>

/**
 * @brief 視覚化するサウンドイベントの種類
 */
enum class SoundVisualType
{
    Gunshot,    ///< 敵の銃声（赤橙色の弾丸/マズル風マーカー）
    SonarPing   ///< 敵の定期音波探知（水色の電波音波パルス風マーカー）
};

/**
 * @brief 個別の音インジケーター情報
 */
struct SoundIndicator
{
    Vector2 worldPos;       ///< 音の発生したワールド座標
    SoundVisualType type;   ///< 音の種類
    int lifeTimer;          ///< 残り表示フレーム
    int maxLife;            ///< 初期フレーム数（フェード用）
    float maxDistance;      ///< 最大可聴距離
};

/**
 * @brief 音無し（ミュート）環境でも敵の方向・距離を察知できるサウンドビジュアライザー
 * @details 荒野行動やFortniteのように、自機の周りに音源の方向・距離を示すアーク・アイコンを描画します。
 */
class SoundVisualizer
{
public:
    static SoundVisualizer& GetInstance()
    {
        static SoundVisualizer instance;
        return instance;
    }

    /**
     * @brief 新たな音イベントを登録
     * @param type 音の種類（Gunshot または SonarPing）
     * @param sourcePos 音の発生源のワールド座標
     * @param maxDistance 最大可聴距離
     */
    void AddSound(SoundVisualType type, const Vector2& sourcePos, float maxDistance = 1200.0f);

    /**
     * @brief 毎フレームの更新処理（寿命の減算、消去）
     */
    void Update();

    /**
     * @brief 自機周辺へのインジケーター描画処理
     * @param player 自機プレイヤーポインタ（ライト範囲判定用）
     * @param playerScreenX 自機の画面X座標
     * @param playerScreenY 自機の画面Y座標
     */
    void Draw(const class Player* player, float playerScreenX, float playerScreenY);

    /**
     * @brief 自機周辺へのインジケーター描画処理（後方互換・ワールド座標指定版）
     */
    void Draw(const Vector2& playerWorldPos, float playerScreenX, float playerScreenY);

    /**
     * @brief 全インジケーターのクリア（リスタート時等）
     */
    void Clear();

private:
    SoundVisualizer() = default;
    ~SoundVisualizer() = default;
    SoundVisualizer(const SoundVisualizer&) = delete;
    SoundVisualizer& operator=(const SoundVisualizer&) = delete;

    std::vector<SoundIndicator> m_indicators;
};
