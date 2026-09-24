#pragma once

class Status
{
private:
    // --- ‘Ì—ÍŠÖŒW ---
    int baseMaxHp;
    int itemMaxHpBonus;
    int skillMaxHpBonus;
    int currentHp;

    // --- ‘¬“xŠÖŒW ---
    float baseSpeed;
    float itemSpeedBonus;
    float skillSpeedBonus;
    float speedMultiplier;

    // --- UŒ‚—ÍŠÖŒW ---
    int baseAttack;
    int itemAttackBonus;
    int skillAttackBonus;
    float attackMultiplier;

public:
    Status();
    ~Status() = default;

    // ‰Šú‰»
    void Init(int hp, float speed, int attack);

    // ‘Ì—Í‚Ìæ“¾E‘€ì
    int GetMaxHp() const;
    int GetCurrentHp() const;
    void Heal(int amount);
    void TakeDamage(int amount);
    bool IsDead() const;
    
    void SetItemMaxHpBonus(int bonus);
    void SetSkillMaxHpBonus(int bonus);

    // ‘¬“x‚Ìæ“¾E‘€ì
    float GetSpeed() const;
    void SetBaseSpeed(float speed);
    void SetItemSpeedBonus(float bonus);
    void SetSkillSpeedBonus(float bonus);
    void SetSpeedMultiplier(float multiplier);

    // UŒ‚—Í‚Ìæ“¾E‘€ì
    int GetAttack() const;
    void SetBaseAttack(int attack);
    void SetItemAttackBonus(int bonus);
    void SetSkillAttackBonus(int bonus);
    void SetAttackMultiplier(float multiplier);
};
