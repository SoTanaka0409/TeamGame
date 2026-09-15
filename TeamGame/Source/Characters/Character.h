#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Status.h"
#include "../Skills/SkillData.h"

class Character : public Object2D
{
  protected:
    class ColliderManager *myColliderManager;
    CircleCollider *collider; // 今回は標準でCircleを持つように
    float radius;

    // --- スキル効果（バフ・デバフ）の管理 ---
    struct ActiveEffect {
        SkillMinorTag tag;
        int remainingFrames;
        float value;
    };
    std::vector<ActiveEffect> activeEffects;

  public:
    Status status;

    Character(ObjectTag tag, float startX, float startY, float radius);
    virtual ~Character();

    CircleCollider *GetCollider() const
    {
        return collider;
    }

    void AddActiveEffect(SkillMinorTag tag, int duration, float value = 0.0f)
    {
        activeEffects.push_back({tag, duration, value});
    }

    // 毎フレーム呼び出す想定
    void UpdateActiveEffects()
    {
        for (auto it = activeEffects.begin(); it != activeEffects.end(); )
        {
            it->remainingFrames--;
            if (it->remainingFrames <= 0)
            {
                // 効果が切れたときの元に戻す処理
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
