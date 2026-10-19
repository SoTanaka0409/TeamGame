#pragma once
#include <unordered_map>
#include "DxLib.h"

/**
 * @brief キャラクターのモーション種別
 */
enum class MotionType
{
    Idle,   ///< 待機モーション (デフォルト: 0)
    Walk,   ///< 歩行モーション (デフォルト: 1)
    Shoot,  ///< 射撃・攻撃モーション (デフォルト: 待機または専用)
    Die     ///< 死亡・やられモーション (デフォルト: 2)
};

/**
 * @brief 3Dモデルのアニメーション管理クラス
 * @details アニメーションのアタッチ、再生、ループ、進行時間を専門に管理します。
 */
class Animation
{
private:
    int m_modelHandle;                          ///< 対象モデルハンドル
    int m_attachIndex;                          ///< DxLibアタッチ識別子
    MotionType m_currentMotion;                 ///< 現在再生中のモーション
    int m_currentAnimIndex;                     ///< 現在のMV1アニメーション番号
    float m_animTime;                           ///< 現在の再生時間
    float m_totalAnimTime;                      ///< アニメーション総時間
    float m_playSpeed;                          ///< 再生速度倍率
    bool m_isLoop;                              ///< ループ再生フラグ
    bool m_isFinished;                          ///< 再生終了フラグ

    std::unordered_map<MotionType, int> m_motionMap; ///< モーション種別とモデル内番号の対応表

public:
    Animation(int modelHandle = -1);
    ~Animation();

    // コピー禁止
    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;

    /**
     * @brief モデルハンドルの設定
     */
    void SetModelHandle(int modelHandle);

    /**
     * @brief モーション種別とアニメーション番号の対応関係を登録
     */
    void SetMotionIndex(MotionType motion, int animIndex);

    /**
     * @brief 指定したモーションを再生
     * @param motion 再生したいモーション種別
     * @param loop ループ再生するか
     * @param speed 再生速度倍率
     */
    void Play(MotionType motion, bool loop = true, float speed = 1.0f);

    /**
     * @brief アニメーション番号を直接指定して再生
     */
    void PlayByIndex(int animIndex, bool loop = true, float speed = 1.0f);

    /**
     * @brief アニメーション時間の更新（毎フレーム呼ぶ）
     */
    void Update(float deltaTime = 1.0f);

    /**
     * @brief 再生が終了したか確認（ループ時は常にfalse）
     */
    bool IsFinished() const { return m_isFinished; }

    /**
     * @brief 現在再生中のモーション種別の取得
     */
    MotionType GetCurrentMotion() const { return m_currentMotion; }

    /**
     * @brief 現在のアニメーション番号の取得
     */
    int GetCurrentAnimIndex() const { return m_currentAnimIndex; }
};
