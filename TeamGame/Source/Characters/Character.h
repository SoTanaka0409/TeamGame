#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Status.h"
#include "../Skills/SkillData.h"
#include <memory>
#include <vector>

#include "../Skills/Skill.h"

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

public:
    Status status;                ///< キャラクターのステータス
    int teamId;                   ///< 所属するチームID
    int equippedSkillId = 1;      ///< 現在指定されているスキルのID
    bool isDeadProcessed = false;  ///< 死亡処理フラグ
    int respawnTimer = 0;         ///< リスポーンタイマー
    int invincibleTimer = 0;      ///< 無敵時間タイマー

    bool IsInBush() const { return m_isInBush; }
    void SetInBush(bool val) { m_isInBush = val; }

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
     * @brief 持続バフ・デバフ効果の追加
     */
    void AddActiveEffect(SkillMinorTag tag, int durationFrames, float value);

    /**
     * @brief 持続効果の更新・解除
     */
    void UpdateActiveEffects();
};
