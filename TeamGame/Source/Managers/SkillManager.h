#pragma once

#include "../Skills/Skill.h"
#include "../Skills/SkillData.h"

class Character;

/**
 * @brief キャラクターのスキル使用リクエストの窓口を担当するマネージャークラス
 */
class SkillManager {
private:
    SkillManager() = default;
    ~SkillManager() = default;

public:
    static SkillManager& GetInstance();

    SkillManager(const SkillManager&) = delete;
    SkillManager& operator=(const SkillManager&) = delete;

    /**
     * @brief キャラクターが自身の所有するスキルを発動
     * @param user 発動を試みるキャラクターポインタ
     * @return bool 発動成功時 true
     */
    bool TriggerSkill(Character* user);

    /**
     * @brief スキルIDを指定してキャラクターにスキルを発動させる
     */
    bool TriggerSkill(int skillId, Character* user);

    /**
     * @brief 全スキル管理のリセット（互換用）
     */
    void Update() {}
    void Clear() {}
};
