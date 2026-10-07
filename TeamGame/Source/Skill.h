#pragma once
#include"Skills/SkillData.h"
#include"Character.h"

class Skill
{
  public:
    Skill();
    ~Skill();
    // 時間経過でクールタイムを減らす
    void UpdateTimer(Character *character, int deltaTime);

    void ApplyEffect(Character *character, const SkillData &skillData);

    bool IsSkillReady(Character *character, const SkillData &skillData) const;

};

