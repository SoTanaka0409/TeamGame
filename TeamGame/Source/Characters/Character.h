#pragma once
#include "CircleCollider.h"
#include "Object2D.h"
#include "Status.h"
#include "../Skills/SkillData.h"

class Character : public Object2D
{
  protected:
    class ColliderManager *myColliderManager;
    CircleCollider *collider; // 莉雁屓縺ｯ讓呎ｺ悶〒Circle繧呈戟縺､繧医≧縺ｫ
    float radius;

    // --- 繧ｹ繧ｭ繝ｫ蜉ｹ譫懶ｼ医ヰ繝輔・繝・ヰ繝包ｼ峨・邂｡逅・---
    struct ActiveEffect {
        SkillMinorTag tag;
        int remainingFrames;
        float value;
    };
    std::vector<ActiveEffect> activeEffects;

  public:
    Status status;
    int teamId;
    bool isDeadProcessed = false;
    int respawnTimer = 0;
    int invincibleTimer = 0;

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

    // 豈弱ヵ繝ｬ繝ｼ繝蜻ｼ縺ｳ蜃ｺ縺呎Φ螳・
    void UpdateActiveEffects()
    {
        for (auto it = activeEffects.begin(); it != activeEffects.end(); )
        {
            it->remainingFrames--;
            if (it->remainingFrames <= 0)
            {
                // 蜉ｹ譫懊′蛻・ｌ縺溘→縺阪・蜈・↓謌ｻ縺吝・逅・
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
