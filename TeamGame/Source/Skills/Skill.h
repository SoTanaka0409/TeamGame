#pragma once
#include <string>

class Character;

class Skill
{
protected:
    std::string skillName;
    int coolTimeMax;
    int coolTimeTimer;
    int mpCost; // MPなどの消費リソースを想定

public:
    Skill(const std::string& name, int coolTime, int cost)
        : skillName(name), coolTimeMax(coolTime), coolTimeTimer(0), mpCost(cost)
    {
    }

    virtual ~Skill() {}

    virtual void Update()
    {
        if (coolTimeTimer > 0)
        {
            coolTimeTimer--;
        }
    }

    virtual bool CanUse(Character* user) const
    {
        // 必要に応じてステータスのMPやスタミナが足りているかの判定を追加します
        return coolTimeTimer <= 0;
    }

    // スキルの発動処理
    virtual void Use(Character* user)
    {
        if (CanUse(user))
        {
            Execute(user);
            coolTimeTimer = coolTimeMax;
        }
    }

protected:
    // 継承先の各スキルで具体的な効果を実装する
    virtual void Execute(Character* user) = 0;

public:
    std::string GetName() const { return skillName; }
    int GetCoolTimeTimer() const { return coolTimeTimer; }
    int GetCoolTimeMax() const { return coolTimeMax; }
    int GetMpCost() const { return mpCost; }
};
