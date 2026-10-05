#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Status.h"
#include "../Skills/SkillData.h"

/**
 * @brief キャラクターの基底クラス
 * @details プレイヤーや敵など、ゲーム内のすべてのキャラクターの共通処理を管理します。
 * 衝突判定やステータス、アクティブなエフェクト（バフ・デバフ）などを保持します。
 */
class Character : public Object2D
{
  protected:
    class ColliderManager *myColliderManager; ///< コライダーを管理するマネージャーへのポインタ
    CircleCollider *collider; ///< 衝突判定用の円形コライダー
    float radius; ///< キャラクターの半径

    /**
     * @brief アクティブなエフェクト（バフ・デバフ）を管理する構造体
     */
    struct ActiveEffect {
        SkillMinorTag tag;    ///< エフェクトの種類
        int remainingFrames;  ///< 残りフレーム数
        float value;          ///< 効果値
    };
    std::vector<ActiveEffect> activeEffects; ///< 現在かかっているエフェクトのリスト

  protected:
    bool m_isInBush = false; ///< ブッシュ潜伏状態

  public:
    Status status; ///< キャラクターのステータス
    int teamId; ///< 所属するチームID
    bool isDeadProcessed = false; ///< 死亡処理が済んでいるかどうかのフラグ
    int respawnTimer = 0; ///< リスポーンまでのタイマー
    int invincibleTimer = 0; ///< 無敵時間のタイマー

    /**
     * @brief ブッシュに潜伏中かどうか取得
     */
    bool IsInBush() const { return m_isInBush; }

    /**
     * @brief ブッシュ潜伏状態を設定
     */
    void SetInBush(bool val) { m_isInBush = val; }

    /**
     * @brief コンストラクタ
     * @param tag オブジェクトのタグ（プレイヤー、敵など）
     * @param startX 初期X座標
     * @param startY 初期Y座標
     * @param radius キャラクターの衝突半径
     */
    Character(ObjectTag tag, float startX, float startY, float radius);

    /**
     * @brief デストラクタ
     */
    virtual ~Character();

    /**
     * @brief キャラクターのコライダーを取得する
     * @return CircleColliderへのポインタ
     */
    CircleCollider *GetCollider() const
    {
        return collider;
    }

    /**
     * @brief 新しいエフェクト（バフ・デバフ）を追加する
     * @param tag エフェクトの種類
     * @param duration 持続時間（フレーム数）
     * @param value 効果値（デフォルトは0.0f）
     */
    void AddActiveEffect(SkillMinorTag tag, int duration, float value = 0.0f)
    {
        activeEffects.push_back({tag, duration, value});
    }

    /**
     * @brief アクティブなエフェクトの更新処理
     * @details 毎フレーム呼び出され、持続時間が切れたエフェクトを解除します。
     */
    void UpdateActiveEffects()
    {
        for (auto it = activeEffects.begin(); it != activeEffects.end(); )
        {
            it->remainingFrames--;
            if (it->remainingFrames <= 0)
            {
                // 効果が切れた時の処理
                if (it->tag == SkillMinorTag::AttackUp) {
                    status.SetSkillAttackBonus(0); 
                } else if (it->tag == SkillMinorTag::SpeedUp) {
                    status.SetSkillSpeedBonus(0);
                }
                it = activeEffects.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
};
