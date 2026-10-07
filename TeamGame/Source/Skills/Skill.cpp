#include "Skill.h"
#include "../Characters/Character.h"
#include "../Managers/ObjectManager.h"
#include "../Managers/SceneManager.h"
#include "../Scenes/Scene.h"
#include <iostream>

Skill::Skill(const SkillData* skillData)
    : data(skillData), coolTimeTimer(0)
{
}

void Skill::Update()
{
    if (coolTimeTimer > 0)
    {
        coolTimeTimer--;
    }
}

bool Skill::CanUse(const Character* user) const
{
    if (!user || user->status.IsDead() || !data) return false;
    return coolTimeTimer <= 0;
}

void Skill::Use(Character* user)
{
    if (!CanUse(user) || !data) return;

    std::cout << "[Skill Execution] SkillID: " << data->id 
              << " 「" << data->name << "」 を発動!" 
              << " 発動者位置: (" << user->GetPosition().x << ", " << user->GetPosition().y << ")" << std::endl;

    switch (data->majorTag)
    {
    case SkillMajorTag::StatusBuff:
        ApplyStatusBuff(user);
        break;
    case SkillMajorTag::Debuff:
        ApplyDebuff(user);
        break;
    case SkillMajorTag::Trap:
        break;
    }

    coolTimeTimer = data->coolTime;
}

void Skill::ApplyStatusBuff(Character* user)
{
    if (!user || !data) return;

    if (data->minorTag == SkillMinorTag::Heal)
    {
        int healAmount = static_cast<int>(data->effectValue);
        int prevHp = user->status.GetCurrentHp();
        user->status.Heal(healAmount);
        int afterHp = user->status.GetCurrentHp();

        std::cout << "[Skill Effect: Heal] HPが " << (afterHp - prevHp) 
                  << " 回復しました。 (現在HP: " << afterHp << "/" << user->status.GetMaxHp() << ")" << std::endl;
    }
    else if (data->minorTag == SkillMinorTag::AttackUp || data->minorTag == SkillMinorTag::SpeedUp)
    {
        user->AddActiveEffect(data->minorTag, data->duration, data->effectValue);
    }
}

void Skill::ApplyDebuff(Character* user)
{
    if (!user || !data) return;

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (!scene || !scene->GetObjectManager()) return;

    const auto& objects = scene->GetObjectManager()->GetObjects();
    float effectRadius = data->effectValue;
    float radiusSq = effectRadius * effectRadius;
    int hitCount = 0;

    for (auto* obj : objects)
    {
        if (!obj || !obj->IsActive()) continue;

        Character* targetChar = dynamic_cast<Character*>(obj);
        if (targetChar && targetChar->teamId != user->teamId && !targetChar->status.IsDead())
        {
            float dx = targetChar->GetPosition().x - user->GetPosition().x;
            float dy = targetChar->GetPosition().y - user->GetPosition().y;
            float distSq = (dx * dx) + (dy * dy);

            if (distSq <= radiusSq)
            {
                targetChar->AddActiveEffect(data->minorTag, data->duration, 0.0f);
                hitCount++;
            }
        }
    }

    std::cout << "[Skill Effect: Debuff] 周囲の敵 " << hitCount << " 体にデバフ効果を付与" << std::endl;
}
