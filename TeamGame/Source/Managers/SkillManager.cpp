#include "SkillManager.h"
#include "../Characters/Character.h"
#include <iostream>

SkillManager& SkillManager::GetInstance() {
    static SkillManager instance;
    return instance;
}

bool SkillManager::TriggerSkill(Character* user) {
    if (!user) return false;

    // キャラクターがまだスキルインスタンスを所有していなければ装備・生成
    Skill* skill = user->GetEquippedSkill();
    if (!skill) {
        user->SetEquippedSkillById(user->equippedSkillId);
        skill = user->GetEquippedSkill();
    }

    if (!skill || !skill->CanUse(user)) {
        return false; // スキル自体の判定で不可 (クールタイム中・死亡中など)
    }

    // スキル自体がUse処理とクールタイムセットを完結
    skill->Use(user);

    return true;
}

bool SkillManager::TriggerSkill(int skillId, Character* user) {
    if (!user) return false;

    // もし指定されたスキルIDが現在の装備と異なれば新しく装備設定
    if (user->equippedSkillId != skillId || !user->GetEquippedSkill()) {
        user->SetEquippedSkillById(skillId);
    }

    return TriggerSkill(user);
}
