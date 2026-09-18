#include "SceneManager.h"
#include "Scene.h"
#pragma once
#include "SkillData.h"
#include "../Characters/Character.h"
#include "../Managers/ObjectManager.h"
#include <cmath>

class Skill
{
private:
    const SkillData* data;
    int coolTimeTimer;

public:
    Skill(const SkillData* skillData) : data(skillData), coolTimeTimer(0) {}
    virtual ~Skill() {}

    void Update()
    {
        if (coolTimeTimer > 0)
        {
            coolTimeTimer--;
        }
    }

    bool CanUse(Character* user) const
    {
        return coolTimeTimer <= 0; // MPのチェック等もここに追加可能
    }

    void Use(Character* user)
    {
        if (!CanUse(user) || !data) return;

        switch (data->majorTag)
        {
        case SkillMajorTag::StatusBuff:
            ApplyStatusBuff(user);
            break;
        case SkillMajorTag::Debuff:
            ApplyDebuff(user);
            break;
        case SkillMajorTag::Trap:
            // トラップの処理をここに追加
            break;
        }

        coolTimeTimer = data->coolTime;
    }

private:
    void ApplyStatusBuff(Character* user)
    {
        if (data->minorTag == SkillMinorTag::Heal)
        {
            user->status.Heal(static_cast<int>(data->effectValue));
        }
        else if (data->minorTag == SkillMinorTag::AttackUp)
        {
            // 注: 実際のプロジェクトでは、Character側でタイマーリスト(ActiveEffects)を作り、
            // duration(効果時間)が過ぎたら戻す処理が必要です。
            user->status.SetSkillAttackBonus(static_cast<int>(data->effectValue));
            user->AddActiveEffect(data->minorTag, data->duration, data->effectValue);
        }
        else if (data->minorTag == SkillMinorTag::SpeedUp)
        {
            user->status.SetSkillSpeedBonus(data->effectValue);
            user->AddActiveEffect(data->minorTag, data->duration, data->effectValue);
        }
    }

    void ApplyDebuff(Character* user)
    {
        // 円形状の範囲でオブジェクトを検索し、敵にデバフを付与
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (!scene || !scene->GetObjectManager()) return;
        const auto& objects = scene->GetObjectManager()->GetObjects();
        float effectRadius = data->effectValue; // effectValueを効果半径として使用
        float radiusSq = effectRadius * effectRadius;

        for (auto* obj : objects)
        {
            // 自分自身やプレイヤーの攻撃などを除外するため、タグで敵か判定
            if (obj->GetObjectTag() == ObjectTag::Enemy)
            {
                Character* enemy = static_cast<Character*>(obj);
                
                // 距離の二乗で判定
                float dx = enemy->GetPosition().x - user->GetPosition().x;
                float dy = enemy->GetPosition().y - user->GetPosition().y;
                float distanceSq = (dx * dx) + (dy * dy);

                if (distanceSq <= radiusSq)
                {
                    // 範囲内の敵に効果(スタンや目くらまし)を付与
                    enemy->AddActiveEffect(data->minorTag, data->duration, 0.0f);
                }
            }
        }
    }

public:
    const SkillData* GetData() const { return data; }
    int GetCoolTimeTimer() const { return coolTimeTimer; }
};
