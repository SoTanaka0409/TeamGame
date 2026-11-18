#pragma once
#include "../Core/Vector2.h"
#include <string>
#include <vector>

/**
 * @brief プレイヤーの攻撃が当たったときに表示するダメージ数字
 */
struct DamagePopup
{
    const void* targetId;   ///< 当たった相手の識別用（比較のみに使い、中身は参照しない）
    Vector2 worldPos;       ///< 表示を始めたワールド座標
    int amount;             ///< 表示するダメージ量
    bool isKill;            ///< とどめの一撃か
    int life;               ///< 残り表示フレーム
    int maxLife;            ///< 初期表示フレーム（フェード計算用）
    float driftX;           ///< 横方向のずれ（数字が重ならないように少し散らす）
};

/**
 * @brief 当たった場所に一瞬だけ出る「×」印（ヒットマーカー）
 */
struct HitMarker
{
    Vector2 worldPos;       ///< 表示するワールド座標
    bool isKill;            ///< とどめの一撃か（赤く大きく表示）
    int life;               ///< 残り表示フレーム
    int maxLife;            ///< 初期表示フレーム
};

/**
 * @brief 攻撃が当たったときの手ごたえ（ヒットマーカー・ダメージ数字・画面揺れ・ヒットストップ）を管理するクラス
 * @details 見た目と体感だけを変える演出で、ダメージ量や当たり判定などのゲームルールには影響しません。
 *          プレイヤー本人の攻撃が当たったときだけ反応します（味方Botの攻撃では出ません）。
 */
class HitFeedbackManager
{
public:
    /**
     * @brief シングルトンインスタンスの取得
     * @return HitFeedbackManagerの参照
     */
    static HitFeedbackManager& GetInstance();

    /**
     * @brief 表示中の演出をすべて消す（試合開始時に呼ぶ）
     */
    void Clear();

    /**
     * @brief 攻撃した人がプレイヤー本人かどうかを判定する
     * @param attackerName 攻撃した人のキャラクター名
     * @param attackerTeamId 攻撃した人のチームID
     * @return プレイヤー本人の攻撃なら true
     */
    static bool IsLocalPlayerAttack(const std::string& attackerName, int attackerTeamId);

    /**
     * @brief プレイヤーの攻撃が当たったことを知らせる
     * @param targetId 当たった相手（同じ相手への連続ヒットをまとめるための識別用）
     * @param hitPos 当たった場所のワールド座標
     * @param damage 与えたダメージ量
     * @param isKill とどめの一撃なら true
     */
    void OnPlayerHit(const void* targetId, const Vector2& hitPos, int damage, bool isKill);

    /**
     * @brief 毎フレームの更新（タイマーを進め、画面揺れの量を決める）
     */
    void Update();

    /**
     * @brief このフレームはゲーム世界の更新を止めるべきか（ヒットストップ・スロー演出用）
     * @return 止めるなら true
     */
    bool ShouldSkipWorldUpdate() const;

    /**
     * @brief 今の画面揺れの量（画面上のピクセル）
     * @return 揺れのずれ量
     */
    Vector2 GetShakeOffset() const { return m_shakeOffset; }

    /**
     * @brief ヒットマーカーとダメージ数字を描画する（カメラ設定のあとに呼ぶ）
     */
    void Draw() const;

private:
    HitFeedbackManager() = default;
    HitFeedbackManager(const HitFeedbackManager&) = delete;
    HitFeedbackManager& operator=(const HitFeedbackManager&) = delete;

    /**
     * @brief 画面揺れを始める（今より弱い揺れでは上書きしない）
     * @param power 揺れの強さ（ピクセル）
     * @param frames 揺れる長さ（フレーム）
     */
    void StartShake(float power, int frames);

    std::vector<DamagePopup> m_popups;  ///< 表示中のダメージ数字
    std::vector<HitMarker> m_markers;   ///< 表示中のヒットマーカー

    int m_hitStopTimer = 0;     ///< 完全に止める残りフレーム
    int m_slowTimer = 0;        ///< スロー演出の残りフレーム
    int m_frameCount = 0;       ///< スロー演出の間引き用カウンタ

    int m_shakeTimer = 0;       ///< 画面揺れの残りフレーム
    int m_shakeMaxTimer = 0;    ///< 画面揺れの初期フレーム（減衰計算用）
    float m_shakePower = 0.0f;  ///< 画面揺れの強さ
    Vector2 m_shakeOffset;      ///< 今のフレームの揺れのずれ量
};
