#include "Character.h"
#include "ColliderManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"

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
        myColliderManager->AddCollider(collider);
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

    // 2. バフ・デバフ持続効果の更新
    UpdateActiveEffects();
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
    activeEffects.push_back({ tag, durationFrames, value });

    // バフ効果の初期適用
    if (tag == SkillMinorTag::AttackUp)
    {
        status.SetSkillAttackBonus(static_cast<int>(value));
    }
    else if (tag == SkillMinorTag::SpeedUp)
    {
        status.SetSkillSpeedBonus(value);
    }
}

void Character::UpdateActiveEffects()
{
    for (auto it = activeEffects.begin(); it != activeEffects.end(); )
    {
        it->remainingFrames--;
        if (it->remainingFrames > 0 && !status.IsDead())
        {
            ++it;
        }
        else
        {
            // 効果終了時の解除
            if (it->tag == SkillMinorTag::AttackUp)
            {
                status.SetSkillAttackBonus(0);
            }
            else if (it->tag == SkillMinorTag::SpeedUp)
            {
                status.SetSkillSpeedBonus(0.0f);
            }
            it = activeEffects.erase(it);
        }
    }
}
