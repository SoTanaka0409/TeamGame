#define NOMINMAX
#include "Character.h"
#include "ColliderManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"
#include "Enemy.h"
#include "../Managers/EffectManager.h"
#include "../Managers/ObjectManager.h"
#include "../Stage/Stage.h"

Character::Character(ObjectTag tag, float startX, float startY, float rad)
    : Object2D(tag), radius(rad), myColliderManager(nullptr), teamId(-1)
{
    position = Vector2(startX, startY);
    width = rad * 2.0f;
    height = rad * 2.0f;

    collider = new CircleCollider(this, rad, "CharacterBody");

    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene)
    {
        myColliderManager = scene->GetColliderManager();
        if (myColliderManager)
        {
            myColliderManager->AddCollider(collider);
        }
    }
}

Character::~Character()
{
    if (myColliderManager)
    {
        myColliderManager->RemoveCollider(collider);
    }
    delete collider;
}

void Character::Update()
{
    // 無敵タイマーの減衰
    if (invincibleTimer > 0)
    {
        invincibleTimer--;
    }

    // 1. キャラクターが所有するスキルのクールタイムを自身で更新（スキル自体のUpdateを呼び出す）
    if (equippedSkill)
    {
        equippedSkill->Update();
    }
    else if (equippedSkillId > 0)
    {
        // 未生成の場合は自動で所持スキルを生成して装備
        SetEquippedSkillById(equippedSkillId);
    }

    // 1.5 吹き飛ばしノックバックの減衰移動と壁判定
    float kbLenSq = knockbackVelocity.x * knockbackVelocity.x + knockbackVelocity.y * knockbackVelocity.y;
    if (kbLenSq > 0.01f)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        const Stage* stage = scene ? scene->GetStage() : nullptr;
        float cellSize = 75.0f; // 標準セルサイズ

        float nextX = position.x + knockbackVelocity.x;
        float nextY = position.y + knockbackVelocity.y;

        if (stage)
        {
            float r = radius * 0.8f;
            // X方向の壁チェック
            int minGx = static_cast<int>((nextX - r) / cellSize);
            int maxGx = static_cast<int>((nextX + r) / cellSize);
            int minGy = static_cast<int>((position.y - r) / cellSize);
            int maxGy = static_cast<int>((position.y + r) / cellSize);

            bool xBlocked = false;
            for (int gx = minGx; gx <= maxGx && !xBlocked; ++gx) {
                for (int gy = minGy; gy <= maxGy && !xBlocked; ++gy) {
                    if (stage->IsSolidWall(gx, gy)) xBlocked = true;
                }
            }
            if (!xBlocked) {
                position.x = nextX;
            } else {
                knockbackVelocity.x = 0.0f;
            }

            // Y方向の壁チェック
            minGx = static_cast<int>((position.x - r) / cellSize);
            maxGx = static_cast<int>((position.x + r) / cellSize);
            minGy = static_cast<int>((nextY - r) / cellSize);
            maxGy = static_cast<int>((nextY + r) / cellSize);

            bool yBlocked = false;
            for (int gx = minGx; gx <= maxGx && !yBlocked; ++gx) {
                for (int gy = minGy; gy <= maxGy && !yBlocked; ++gy) {
                    if (stage->IsSolidWall(gx, gy)) yBlocked = true;
                }
            }
            if (!yBlocked) {
                position.y = nextY;
            } else {
                knockbackVelocity.y = 0.0f;
            }
        }
        else
        {
            position.x = nextX;
            position.y = nextY;
        }

        // 摩擦による急激な減衰
        knockbackVelocity.x *= 0.82f;
        knockbackVelocity.y *= 0.82f;
        if (std::abs(knockbackVelocity.x) < 0.1f) knockbackVelocity.x = 0.0f;
        if (std::abs(knockbackVelocity.y) < 0.1f) knockbackVelocity.y = 0.0f;
    }

    // 2. バフ・デバフ持続効果の更新
    UpdateActiveEffects();

    // 2.1 HighRiskスキル発動中の定期的な音波パルス＆敵への通知
    if (IsHighRisk() && !status.IsDead())
    {
        highRiskSoundTimer++;
        if (highRiskSoundTimer >= 60) // 1秒(60フレーム)毎に音波パルスを放射
        {
            highRiskSoundTimer = 0;
            auto scene = SceneManager::GetInstance().GetCurrentScene();
            if (scene)
            {
                if (scene->GetEffectManager())
                {
                    // 赤いソナー音波エフェクトを表示
                    scene->GetEffectManager()->AddSonarWaveEffect(position.x, position.y, GetColor(255, 60, 60));
                }
                if (scene->GetObjectManager())
                {
                    // 敵キャラクター全員に音を感知させる
                    for (auto* obj : scene->GetObjectManager()->GetObjects())
                    {
                        if (obj && obj->IsActive())
                        {
                            Enemy* e = dynamic_cast<Enemy*>(obj);
                            if (e && e->teamId != this->teamId)
                            {
                                e->OnHearGunshot(position, 1200.0f, this->teamId);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        highRiskSoundTimer = 0;
    }

    // 3. 3Dモデル・アニメーションの更新
    if (m_model)
    {
        m_model->Update();
    }
}

void Character::SetColliderRadius(float rad)
{
    if (rad <= 1.0f) rad = 8.9f;
    radius = rad;
    width = rad * 2.0f;
    height = rad * 2.0f;
    if (collider)
    {
        collider->SetRadius(rad);
    }
}

void Character::SetModelScale(float scale)
{
    if (scale <= 0.001f) scale = 0.3f;
    m_modelScale = scale;
    if (m_model)
    {
        m_model->SetScaleUniform(scale);
    }
}

void Character::SetModelPath(const std::string& path)
{
    if (path.empty())
    {
        m_modelPath.clear();
        m_model.reset();
        return;
    }
    if (m_model && m_modelPath == path)
    {
        return;
    }
    m_modelPath = path;
    m_model = std::make_unique<Model>(path);
    if (m_model && m_modelScale > 0.0f)
    {
        m_model->SetScaleUniform(m_modelScale);
    }
}

void Character::SetEquippedSkill(std::unique_ptr<Skill> skill)
{
    equippedSkill = std::move(skill);
}

void Character::SetEquippedSkillById(int skillId)
{
    equippedSkillId = skillId;
    const SkillData* data = SkillDataManager::GetInstance().GetSkillData(skillId);
    if (data)
    {
        equippedSkill = std::make_unique<Skill>(data);
    }
}

void Character::AddActiveEffect(SkillMinorTag tag, int durationFrames, float value)
{
    const int MAX_STACK_COUNT = 3;

    // 現在同種のタグが何個かかっているかカウント
    int count = 0;
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == tag) count++;
    }

    if (count >= MAX_STACK_COUNT)
    {
        // 最大3スタック到達時: 一番残りタイマーが少ない効果の時間を延長（タイマーリフレッシュ）
        ActiveEffect* oldest = nullptr;
        int minFrames = 999999;
        for (auto& effect : activeEffects)
        {
            if (effect.tag == tag && effect.remainingFrames < minFrames)
            {
                minFrames = effect.remainingFrames;
                oldest = &effect;
            }
        }
        if (oldest)
        {
            oldest->remainingFrames = (std::max)(oldest->remainingFrames, durationFrames);
            oldest->value = value;
        }
    }
    else
    {
        activeEffects.push_back({ tag, durationFrames, value });
    }

    RecalculateActiveEffects();
}

void Character::RecalculateActiveEffects()
{
    int attackBonus = 0;
    float speedMultBonus = 0.0f; // +0.2f (+20%) や -0.3f (-30%)

    for (const auto& effect : activeEffects)
    {
        switch (effect.tag)
        {
        case SkillMinorTag::AttackUp:
            attackBonus += static_cast<int>(effect.value);
            break;
        case SkillMinorTag::AttackDown:
            attackBonus -= static_cast<int>(effect.value);
            break;
        case SkillMinorTag::SpeedUp:
            speedMultBonus += effect.value;
            break;
        case SkillMinorTag::SpeedDown:
            speedMultBonus -= effect.value;
            break;
        default:
            break;
        }
    }

    // 攻撃力は固定値の加算・減算
    status.SetSkillAttackBonus(attackBonus);

    // 移動速度は％倍率（基本倍率1.0fに加減算、下限は0.1f）
    float finalSpeedMult = 1.0f + speedMultBonus;
    if (finalSpeedMult < 0.1f) finalSpeedMult = 0.1f;
    status.SetSpeedMultiplier(finalSpeedMult);
}

void Character::UpdateActiveEffects()
{
    bool needsRecalc = false;
    for (auto it = activeEffects.begin(); it != activeEffects.end(); )
    {
        it->remainingFrames--;
        if (it->remainingFrames > 0 && !status.IsDead())
        {
            ++it;
        }
        else
        {
            it = activeEffects.erase(it);
            needsRecalc = true;
        }
    }

    if (needsRecalc)
    {
        RecalculateActiveEffects();
    }
}

bool Character::IsBlind() const
{
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == SkillMinorTag::Blind && effect.remainingFrames > 0) return true;
    }
    return false;
}

bool Character::IsStunned() const
{
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == SkillMinorTag::Stun && effect.remainingFrames > 0) return true;
    }
    return false;
}

bool Character::CanAction() const
{
    return !IsStunned() && !status.IsDead();
}

float Character::GetSightMultiplier() const
{
    return IsBlind() ? 0.3f : 1.0f;
}

bool Character::IsTransparent() const
{
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == SkillMinorTag::Transparency && effect.remainingFrames > 0) return true;
    }
    return false;
}

bool Character::IsPanic() const
{
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == SkillMinorTag::PanicShoot && effect.remainingFrames > 0) return true;
    }
    return false;
}

bool Character::IsHighRisk() const
{
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == SkillMinorTag::HighRisk && effect.remainingFrames > 0) return true;
    }
    return false;
}

int Character::GetEffectRemainingFrames(SkillMinorTag tag) const
{
    for (const auto& effect : activeEffects)
    {
        if (effect.tag == tag && effect.remainingFrames > 0) return effect.remainingFrames;
    }
    return 0;
}


