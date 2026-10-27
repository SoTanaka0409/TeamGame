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

    // --- 4. Clean time: 周囲のあらゆる物（敵・味方・弾丸）を強力に吹き飛ばす（突風・風の衝撃波） ---
    if (data->minorTag == SkillMinorTag::CleanTime)
    {
        if (scene->GetEffectManager())
        {
            // 風の衝撃波リングと四方八方へ吹き抜ける突風気流エフェクト
            scene->GetEffectManager()->AddWindShockwaveEffect(user->GetPosition().x, user->GetPosition().y, effectRadius);
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

    // --- 3. Panic shoot（バトルロワイヤル：プレイヤー以外の全Botを全員敵同士・単独孤立化） ---
    if (data->minorTag == SkillMinorTag::PanicShoot)
    {
        int panicCount = 0;
        for (auto* obj : objects)
        {
            if (!obj || !obj->IsActive() || obj == user) continue;

            Character* targetChar = dynamic_cast<Character*>(obj);
            if (targetChar && !targetChar->status.IsDead())
            {
                // 生存中のプレイヤー以外の全Bot（味方Bot・敵Bot全員）に錯乱を付与
                targetChar->AddActiveEffect(data->minorTag, data->duration, data->effectValue);
                panicCount++;

                // 各Botの足元・頭上に錯乱発生パルス波を発生
                if (scene->GetEffectManager())
                {
                    scene->GetEffectManager()->AddSonarWaveEffect(targetChar->GetPosition().x, targetChar->GetPosition().y, GetColor(255, 30, 180));
                }
            }
        }

        if (scene->GetEffectManager())
        {
            // 発動者周囲に特大の紫・ピンク・赤ショックウェーブを展開
            scene->GetEffectManager()->AddSonarWaveEffect(user->GetPosition().x, user->GetPosition().y, GetColor(255, 0, 220));
            scene->GetEffectManager()->AddSonarWaveEffect(user->GetPosition().x, user->GetPosition().y, GetColor(200, 40, 255));
            scene->GetEffectManager()->AddSonarWaveEffect(user->GetPosition().x, user->GetPosition().y, GetColor(255, 80, 50));
        }

        std::cout << "[Skill Effect: Battle Royale] 全マップのBot " << panicCount << " 体をバトルロワイヤル状態（全員孤立・敵同士）に移行!" << std::endl;
        return;
    }

    // その他Debuff（Flashbangなど汎用）
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

    std::cout << "[Skill Effect: Debuff] 周囲の敵 " << hitCount << " 体にデバフ効果を付与" << std::endl;
}
