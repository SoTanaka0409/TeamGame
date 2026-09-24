#include "Status.h"
#include <algorithm>

Status::Status()
{
    Init(1, 1.0f, 1);
}

void Status::Init(int hp, float speed, int attack)
{
    baseMaxHp = hp;
    itemMaxHpBonus = 0;
    skillMaxHpBonus = 0;
    currentHp = GetMaxHp(); // ボーナス反映後の最大HPを満タンにする

    baseSpeed = speed;
    itemSpeedBonus = 0.0f;
    skillSpeedBonus = 0.0f;
    speedMultiplier = 1.0f;

    baseAttack = attack;
    itemAttackBonus = 0;
    skillAttackBonus = 0;
    attackMultiplier = 1.0f;
}

int Status::GetMaxHp() const
{
    return baseMaxHp + itemMaxHpBonus + skillMaxHpBonus;
}

int Status::GetCurrentHp() const
{
    return currentHp;
}

void Status::Heal(int amount)
{
    currentHp += amount;
    int maxHp = GetMaxHp();
    if (currentHp > maxHp)
    {
        currentHp = maxHp;
    }
}

void Status::TakeDamage(int amount)
{
    currentHp -= amount;
    if (currentHp < 0)
    {
        currentHp = 0;
    }
}

bool Status::IsDead() const
{
    return currentHp <= 0;
}

void Status::SetItemMaxHpBonus(int bonus)
{
    itemMaxHpBonus = bonus;
    // 最大HPが変わるので現在HPも補正（最大を超えないように）
    if (currentHp > GetMaxHp()) {
        currentHp = GetMaxHp();
    }
}

void Status::SetSkillMaxHpBonus(int bonus)
{
    skillMaxHpBonus = bonus;
    if (currentHp > GetMaxHp()) {
        currentHp = GetMaxHp();
    }
}

float Status::GetSpeed() const
{
    return (baseSpeed + itemSpeedBonus + skillSpeedBonus) * speedMultiplier;
}

void Status::SetBaseSpeed(float speed)
{
    baseSpeed = speed;
}

void Status::SetItemSpeedBonus(float bonus)
{
    itemSpeedBonus = bonus;
}

void Status::SetSkillSpeedBonus(float bonus)
{
    skillSpeedBonus = bonus;
}

void Status::SetSpeedMultiplier(float multiplier)
{
    speedMultiplier = multiplier;
}

int Status::GetAttack() const
{
    return static_cast<int>((baseAttack + itemAttackBonus + skillAttackBonus) * attackMultiplier);
}

void Status::SetBaseAttack(int attack)
{
    baseAttack = attack;
}

void Status::SetItemAttackBonus(int bonus)
{
    itemAttackBonus = bonus;
}

void Status::SetSkillAttackBonus(int bonus)
{
    skillAttackBonus = bonus;
}

void Status::SetAttackMultiplier(float multiplier)
{
    attackMultiplier = multiplier;
}
