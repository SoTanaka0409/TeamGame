#pragma once

#include "SkillData.h"
#include <cmath>

class Character;

/**
 * @brief 各キャラクターが保持・使用するスキル実体クラス
 * @details スキルごとの個人用クールタイム(coolTimeTimer)の管理と、発動処理(Use)を行います。
 */
class Skill
{
private:
    const SkillData* data;   ///< CSVから読み込まれた静的マスターデータへの参照
    int coolTimeTimer;       ///< このキャラ個人用の残りクールタイム時間 (フレーム数)

    void ApplyStatusBuff(Character* user);
    void ApplyDebuff(Character* user);

public:
    explicit Skill(const SkillData* skillData);
    virtual ~Skill() = default;

    virtual void Update();
    virtual bool CanUse(const Character* user) const;
    virtual void Use(Character* user);

    const SkillData* GetData() const { return data; }
    int GetCoolTimeTimer() const { return coolTimeTimer; }
};
