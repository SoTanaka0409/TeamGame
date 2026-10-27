#include "PlayerSelection.h"

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
    return {
        { 1, "指揮者", "Tactical", "味方Botを敵の手前で爆発させる [Homing Kill] を操る冷徹な指揮者",  37, 4.2f, 1, "Homing Kill", 0xAA44FF, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_1.png" },
        { 2, "反逆者", "Speed",    "敵味方双方から姿を消す [Transparency] を持つ神出鬼没な反逆者",   28, 5.2f, 2, "Transparency", 0x00A2FF, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_2.png" },
        { 3, "バカ",   "Assault",  "周囲を錯乱させ同士討ちを起こす [Panic shoot] を放つ暴走アタッカー",  33, 4.4f, 3, "Panic shoot", 0x00FF88, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_3.png" },
        { 4, "主婦",   "Heavy",    "周囲の物や弾を綺麗サッパリ吹き飛ばす [Clean time] を持つ最強の主婦",  53, 3.2f, 4, "Clean time", 0xFF4444, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_4.png" },
        { 5, "悪魔",   "Sniper",   "敵の位置を暴くが自身も音を放つ [High risk, high return] を宿した悪魔", 30, 3.8f, 5, "High risk", 0xFFBB00, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png", 8.9f, 0.3f, "Resource/Cutin/cutin_5.png" }
    };
}

WeaponSelectData PlayerSelectionManager::GetFixedSlot1Weapon() {
    return { 101, "Handgun", "Handgun", "Standard Handgun", 12, 25 };
}

std::vector<WeaponSelectData> PlayerSelectionManager::GetAvailableSlot2Weapons() {
    return {
        { 201, "Shotgun",         "Shotgun", "近接拡散・高威力散弾銃", 6, 18 },
        { 202, "Sniper Rifle",    "Sniper",  "超長射程・高速貫通狙撃銃", 5, 80 },
        { 203, "Submachine Gun",  "SMG",     "高速連射短機関銃 (★キャラ別カスタム弾＆銃身！)", 32, 10 }
    };
}
