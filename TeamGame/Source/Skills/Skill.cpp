#include "Skill.h"
#include "../Characters/Character.h"
#include "../Characters/Enemy.h"
#include "../Weapons/Bullet.h"
#include "../Managers/EffectManager.h"
#include "../Managers/ObjectManager.h"
#include "../Managers/SceneManager.h"
#include "../Scenes/Scene.h"
#include <iostream>
#include <cmath>

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
    else if (data->minorTag == SkillMinorTag::Transparency || data->minorTag == SkillMinorTag::HighRisk)
    {
        user->AddActiveEffect(data->minorTag, data->duration, data->effectValue);
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

    // --- 1. Homing Kill: 味方Botを敵の手前で爆発させて殺す ---
    if (data->minorTag == SkillMinorTag::HomingKill)
    {
        // 1. 最も近い味方Bot (EnemyでteamIdが同じで特攻中でないもの) を探す
        Enemy* closestAllyBot = nullptr;
        float minAllyDistSq = 999999.0f;
        for (auto* obj : objects)
        {
            if (!obj || !obj->IsActive()) continue;
            Enemy* ally = dynamic_cast<Enemy*>(obj);
            if (ally && ally != user && ally->teamId == user->teamId && !ally->status.IsDead() && !ally->IsHomingSuicide())
            {
                float dx = ally->GetPosition().x - user->GetPosition().x;
                float dy = ally->GetPosition().y - user->GetPosition().y;
                float distSq = dx * dx + dy * dy;
                if (distSq < minAllyDistSq)
                {
                    minAllyDistSq = distSq;
                    closestAllyBot = ally;
                }
            }
        }

        // 2. 最も近い敵を探す
        Character* closestEnemy = nullptr;
        float minEnemyDistSq = 999999.0f;
        for (auto* obj : objects)
        {
            if (!obj || !obj->IsActive()) continue;
            Character* targetChar = dynamic_cast<Character*>(obj);
            if (targetChar && targetChar->teamId != user->teamId && targetChar->teamId != -1 && !targetChar->status.IsDead())
            {
                float dx = targetChar->GetPosition().x - user->GetPosition().x;
                float dy = targetChar->GetPosition().y - user->GetPosition().y;
                float distSq = dx * dx + dy * dy;
                if (distSq < minEnemyDistSq)
                {
                    minEnemyDistSq = distSq;
                    closestEnemy = targetChar;
                }
            }
        }

        if (closestAllyBot && closestEnemy)
        {
            closestAllyBot->StartHomingSuicide(closestEnemy, 2.0f);
            if (scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddSonarWaveEffect(closestAllyBot->GetPosition().x, closestAllyBot->GetPosition().y, GetColor(255, 100, 0));
            }
            std::cout << "[Skill: Homing Kill] 味方Botを敵へ特攻突撃させました！" << std::endl;
        }
        else
        {
            std::cout << "[Skill: Homing Kill] 味方Botまたは敵が見つかりませんでした" << std::endl;
        }
        return;
    }

    // --- 4. Clean time: 周囲のあらゆる物（敵・味方・弾丸）を強力に吹き飛ばす ---
    if (data->minorTag == SkillMinorTag::CleanTime)
    {
        if (scene->GetEffectManager())
        {
            scene->GetEffectManager()->AddSonarWaveEffect(user->GetPosition().x, user->GetPosition().y, GetColor(100, 200, 255));
            scene->GetEffectManager()->AddMuzzleFlashEffect(user->GetPosition().x, user->GetPosition().y, 0.0f, effectRadius);
        }

        for (auto* obj : objects)
        {
            if (!obj || !obj->IsActive() || obj == user) continue;

            float dx = obj->GetPosition().x - user->GetPosition().x;
            float dy = obj->GetPosition().y - user->GetPosition().y;
            float distSq = dx * dx + dy * dy;

            if (distSq <= radiusSq && distSq > 0.0001f)
            {
                float dist = std::sqrt(distSq);
                float nx = dx / dist;
                float ny = dy / dist;

                // 弾丸なら即座に消滅・無効化
                if (obj->GetObjectTag() == ObjectTag::PlayerWeapon || obj->GetObjectTag() == ObjectTag::EnemyWeapon)
                {
                    obj->SetActive(false);
                }
                else
                {
                    // キャラクターなら強力なノックバック衝撃インパルスを付与 ＋ スタン
                    Character* chara = dynamic_cast<Character*>(obj);
                    if (chara && !chara->status.IsDead())
                    {
                        float knockbackImpulse = 42.0f; // 毎フレーム減衰しながら猛烈な勢いで吹き飛ぶ
                        chara->ApplyKnockback(Vector2(nx * knockbackImpulse, ny * knockbackImpulse));
                        chara->AddActiveEffect(SkillMinorTag::Stun, 45, 0.0f); // 0.75秒スタン
                    }
                }
            }
        }
        std::cout << "[Skill: Clean time] 周囲 " << effectRadius << "px の物体を強烈に吹き飛ばしました！" << std::endl;
        return;
    }

    // --- 3. Panic shoot / その他Debuff（Flashbangなど） ---
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
                targetChar->AddActiveEffect(data->minorTag, data->duration, data->effectValue);
                hitCount++;
            }
        }
    }

    if (data->minorTag == SkillMinorTag::PanicShoot && scene->GetEffectManager())
    {
        scene->GetEffectManager()->AddSonarWaveEffect(user->GetPosition().x, user->GetPosition().y, GetColor(255, 0, 150));
    }

    std::cout << "[Skill Effect: Debuff] 周囲の敵 " << hitCount << " 体にデバフ効果を付与" << std::endl;
}
