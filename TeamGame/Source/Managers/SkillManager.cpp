#include "SkillManager.h"
#include "../Characters/Character.h"
#include <iostream>

SkillManager& SkillManager::GetInstance() {
    static SkillManager instance;
    return instance;
}

#include "CutinManager.h"
#include "../Characters/Player.h"
#include "../Core/PlayerSelection.h"

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

    // プレイヤーがスキル発動時: ペルソナ5風カットイン演出をトリガー
    Player* player = dynamic_cast<Player*>(user);
    if (player && skill->GetData()) {
        const auto& selection = PlayerSelectionManager::GetInstance().GetSelection();
        std::string cName = selection.selectedCharacter.name.empty() ? "HERO" : selection.selectedCharacter.name;
        std::string sName = skill->GetData()->name;
        unsigned int tColor = player->GetThemeColor();
        std::string previewPath = selection.selectedCharacter.previewPath;
        std::string cutinPath = player->GetCutinImagePath().empty() ? selection.selectedCharacter.cutinImagePath : player->GetCutinImagePath();
        int charaId = player->GetCharacterId();

        CutinManager::GetInstance().StartCutin(charaId, cName, sName, tColor, cutinPath);
    }

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
