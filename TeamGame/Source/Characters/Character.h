#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Status.h"
#include "../Skills/SkillData.h"
#include <memory>
#include <vector>

#include "../Skills/Skill.h"
#include "../Graphics/Model.h"

/**
 * @brief キャラクターの基底クラス
 */
class Character : public Object2D
{
protected:
    class ColliderManager *myColliderManager; ///< コライダーを管理するマネージャーへのポインタ
    CircleCollider *collider;                  ///< 衝突判定用の円形コライダー
    float radius;                              ///< キャラクターの半径

    struct ActiveEffect {
        SkillMinorTag tag;    ///< エフェクトの種類
        int remainingFrames;  ///< 残りフレーム数
        float value;          ///< 効果値
    };
    std::vector<ActiveEffect> activeEffects;   ///< 現在かかっているエフェクトのリスト

    bool m_isInBush = false;                   ///< ブッシュ潜伏状態
    std::unique_ptr<Skill> equippedSkill;      ///< キャラクター自身が保持・所有するスキル実体
    std::string m_modelPath;                   ///< 3Dモデルのリソースパス
    std::unique_ptr<Model> m_model;            ///< 3Dキャラクターモデル・アニメーション管理実体

public:
    Status status;                ///< キャラクターのステータス
    int teamId;                   ///< 所属するチームID
    int equippedSkillId = 1;      ///< 現在指定されているスキルのID
    bool isDeadProcessed = false;  ///< 死亡処理フラグ
    int respawnTimer = 0;         ///< リスポーンタイマー
    int invincibleTimer = 0;      ///< 無敵時間タイマー
    int highRiskSoundTimer = 0;   ///< HighRiskスキル時の定期発音タイマー
    Vector2 knockbackVelocity = Vector2(0.0f, 0.0f); ///< 吹き飛ばし減衰速度ベクトル

    std::string characterName = "Unit";        ///< キャラクター表示名
    std::string lastAttackerName = "";        ///< 最後に攻撃を与えたキャラクター名
    int lastAttackerTeamId = -1;              ///< 最後に攻撃を与えたチームID

    const std::string& GetCharacterName() const { return characterName; }
    void SetCharacterName(const std::string& name) { characterName = name; }
    void SetLastAttacker(const std::string& name, int tId) { lastAttackerName = name; lastAttackerTeamId = tId; }
    const std::string& GetLastAttackerName() const { return lastAttackerName; }
    int GetLastAttackerTeamId() const { return lastAttackerTeamId; }

    bool IsInBush() const { return m_isInBush; }
    void SetInBush(bool val) { m_isInBush = val; }

    /**
     * @brief 吹き飛ばし衝撃力を受ける
     */
    void ApplyKnockback(const Vector2& impulse) {
        knockbackVelocity.x += impulse.x;
        knockbackVelocity.y += impulse.y;
    }

    Character(ObjectTag tag, float startX, float startY, float radius);
    virtual ~Character();

    virtual void Update() override;

    /**
     * @brief スキルの装備・初期化
     */
    void SetEquippedSkill(std::unique_ptr<Skill> skill);
    void SetEquippedSkillById(int skillId);

    /**
     * @brief 自身が所持するスキルインスタンスを取得
     */
    Skill* GetEquippedSkill() const { return equippedSkill.get(); }

    /**
     * @brief 3Dモデルの設定
     */
    void SetModel(std::unique_ptr<Model> model) { m_model = std::move(model); }
    void SetModelPath(const std::string& path);

    /**
     * @brief 3Dモデルの取得
     */
    Model* GetModel() const { return m_model.get(); }
    const std::string& GetModelPath() const { return m_modelPath; }
    bool HasModel() const { return m_model != nullptr && m_model->IsLoaded(); }

    /**
     * @brief 持続バフ・デバフ効果の追加（最大スタック制限あり）
     */
    void AddActiveEffect(SkillMinorTag tag, int durationFrames, float value);

    /**
     * @brief アクティブな効果に基づいてステータス（攻撃力・速度・フラグ等）を全再計算
     */
    void RecalculateActiveEffects();

    /**
     * @brief 持続効果の更新・解除
     */
    void UpdateActiveEffects();

    /**
     * @brief 目くらまし状態か
     */
    bool IsBlind() const;

    /**
     * @brief スタン状態か
     */
    bool IsStunned() const;

    /**
     * @brief 行動可能か（スタン中・死亡中なら false）
     */
    bool CanAction() const;

    /**
     * @brief 有効視界倍率（目くらまし中なら 0.3倍）
     */
    float GetSightMultiplier() const;

    /**
     * @brief ステルス透明化状態か
     */
    bool IsTransparent() const;

    /**
     * @brief パニック（錯乱）状態か
     */
    bool IsPanic() const;

    /**
     * @brief ハイリスク（位置探知＆強制発音）状態か
     */
    bool IsHighRisk() const;

    /**
     * @brief 指定タグのアクティブエフェクトの残りフレーム数を取得（無ければ0）
     */
    int GetEffectRemainingFrames(SkillMinorTag tag) const;

    /**
     * @brief 現在のアクティブエフェクト一覧を取得
     */
    const std::vector<ActiveEffect>& GetActiveEffects() const { return activeEffects; }
};
