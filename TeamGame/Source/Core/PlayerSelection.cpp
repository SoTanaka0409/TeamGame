#include "PlayerSelection.h"
#include "../Managers/CharacterManager.h"
#include "../Skills/SkillData.h"
#include "../Managers/WeaponManager.h"

PlayerSelectionManager& PlayerSelectionManager::GetInstance() {
    static PlayerSelectionManager instance;
    return instance;
}

PlayerSelectionManager::PlayerSelectionManager() {
    auto charas = GetAvailableCharacters();
    if (!charas.empty()) m_selection.selectedCharacter = charas[0];
    m_selection.slot1Weapon = GetFixedSlot1Weapon();
    auto slot2Wpns = GetAvailableSlot2Weapons();
    if (!slot2Wpns.empty()) m_selection.slot2Weapon = slot2Wpns[0];
}

std::vector<SelectionCharacterData> PlayerSelectionManager::GetAvailableCharacters() {
    auto& charaMgr = CharacterManager::GetInstance();
    const auto& allCharas = charaMgr.GetAllCharacters();

    // CSVがまだロードされていない場合は読み込みを試みる
    if (allCharas.empty()) {
        charaMgr.LoadFromCSV("characters.csv");
    }

    const auto& loadedCharas = charaMgr.GetAllCharacters();
    if (!loadedCharas.empty()) {
        std::vector<SelectionCharacterData> result;

        for (const auto& cData : loadedCharas) {
            SelectionCharacterData selData;
            selData.id = cData.id;
            selData.name = cData.name;
            selData.typeName = cData.type;
            selData.description = cData.description;
            selData.maxHp = cData.maxHp;
            selData.moveSpeed = cData.moveSpeed;
            selData.attackPower = cData.attackPower;
            selData.skillId = cData.skillId;
            selData.colliderRadius = cData.colliderRadius;
            selData.modelScale = cData.modelScale;
            selData.modelPath = "Resource/GreenCharacter/character.mv1";
            selData.previewPath = "Resource/GreenCharacter/preview.png";

            // スキル情報の取得
            const SkillData* sData = SkillDataManager::GetInstance().GetSkillData(cData.skillId);
            if (sData) {
                selData.skillName = sData->name;
            } else {
                switch (cData.skillId) {
                case 1: selData.skillName = "Homing Kill"; break;
                case 2: selData.skillName = "Transparency"; break;
                case 3: selData.skillName = "Panic shoot"; break;
                case 4: selData.skillName = "Clean time"; break;
                case 5: selData.skillName = "High risk"; break;
                default: selData.skillName = "Skill " + std::to_string(cData.skillId); break;
                }
            }

            // キャラクターIDに応じたテーマカラーおよびカットイン画像
            switch (cData.id) {
            case 1:
                selData.themeColor = 0xAA44FF;
                selData.cutinImagePath = "Resource/Cutin/cutin_1.png";
                break;
            case 2:
                selData.themeColor = 0x00A2FF;
                selData.cutinImagePath = "Resource/Cutin/cutin_2.png";
                break;
            case 3:
                selData.themeColor = 0x00FF88;
                selData.cutinImagePath = "Resource/Cutin/cutin_3.png";
                break;
            case 4:
                selData.themeColor = 0xFF4444;
                selData.cutinImagePath = "Resource/Cutin/cutin_4.png";
                break;
            case 5:
                selData.themeColor = 0xFFBB00;
                selData.cutinImagePath = "Resource/Cutin/cutin_5.png";
                break;
            default:
                selData.themeColor = 0x00FF88;
                selData.cutinImagePath = "Resource/Cutin/cutin_" + std::to_string(cData.id) + ".png";
                break;
            }

            result.push_back(selData);
        }

        return result;
    }

    // CSV読み込みに失敗した場合の安全フォールバック
    return {
        { 1, "指揮者", "Tactical", "味方Botを特攻自爆させるが敵は無傷、自身もライトが見えず暗闇を彷徨う無能指揮者",  37, 4.2f, 20, 1, "Homing Kill", 0xAA44FF, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_1.png" },
        { 2, "反逆者", "Speed",    "敵味方双方から姿を消す [Transparency] を持つ神出鬼没な反逆者",   28, 5.2f, 18, 2, "Transparency", 0x00A2FF, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_2.png" },
        { 3, "バカ",   "Assault",  "周囲を錯乱させ同士討ちを起こす [Panic shoot] を放つ暴走アタッカー",  33, 4.4f, 25, 3, "Panic shoot", 0x00FF88, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_3.png" },
        { 4, "主婦",   "Heavy",    "周囲の物や弾を綺麗サッパリ吹き飛ばす [Clean time] を持つ最強の主婦",  53, 3.2f, 30, 4, "Clean time", 0xFF4444, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_4.png" },
        { 5, "悪魔",   "Sniper",   "敵の位置を暴くが自身も音を放つ [High risk, high return] を宿した悪魔", 30, 3.8f, 45, 5, "High risk", 0xFFBB00, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_5.png" }
    };
}

WeaponSelectData PlayerSelectionManager::GetFixedSlot1Weapon() {
    auto data = WeaponManager::GetInstance().GetWeaponData("Handgun");
    int dmg = data ? data->damage : 12;
    int ammo = data ? data->maxAmmo : 12;
    return { 101, "Handgun", "Handgun", "高機動・高精度・高速リロードの万能サイドアーム (移動速度+12%UP)", ammo, dmg };
}

std::vector<WeaponSelectData> PlayerSelectionManager::GetAvailableSlot2Weapons() {
    auto sgData = WeaponManager::GetInstance().GetWeaponData("Shotgun");
    int sgDmg = sgData ? (sgData->damage * sgData->pelletCount) : 30;
    int sgAmmo = sgData ? sgData->maxAmmo : 6;

    auto snData = WeaponManager::GetInstance().GetWeaponData("SniperRifle");
    int snDmg = snData ? snData->damage : 36;
    int snAmmo = snData ? snData->maxAmmo : 5;

    auto smgData = WeaponManager::GetInstance().GetWeaponData("SubmachineGun");
    int smgDmg = smgData ? smgData->damage : 3;
    int smgAmmo = smgData ? smgData->maxAmmo : 28;

    return {
        { 201, "Shotgun",         "Shotgun", "至近距離バースト・草むら奇襲（近接制圧散弾）", sgAmmo, sgDmg },
        { 202, "Sniper Rifle",    "Sniper",  "一撃必殺・超長射程・高速貫通狙撃銃", snAmmo, snDmg },
        { 203, "Submachine Gun",  "SMG",     "高速連射短機関銃（弾幕制圧・キャラ別カスタム弾）", smgAmmo, smgDmg }
    };
}
