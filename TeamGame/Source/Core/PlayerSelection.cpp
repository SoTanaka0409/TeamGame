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

std::vector<CharacterData> PlayerSelectionManager::GetAvailableCharacters() {
    return {
        { 1, "Character 1", "アサルトタイプ", "バランスに優れた標準的なキャラクター", 100, 4.2f, 0x00A2FF },
        { 2, "Character 2", "スピードタイプ", "移動速度が速く機動力に長ける", 80, 5.2f, 0x00FF88 },
        { 3, "Character 3", "ヘビータイプ",   "高耐久・高HPでタンク役に最適", 150, 3.4f, 0xFF4444 },
        { 4, "Character 4", "スナイパー",     "長距離からの精密射撃が得意", 90, 4.0f, 0xFFBB00 },
        { 5, "Character 5", "タクティカル",   "特殊スキルと高い旋回力を併せ持つ", 110, 4.5f, 0xAA44FF }
    };
}

WeaponSelectData PlayerSelectionManager::GetFixedSlot1Weapon() {
    return { 101, "Handgun", "Handgun", "標準ハンドガン (スロット1固定装備)", 12, 25 };
}

std::vector<WeaponSelectData> PlayerSelectionManager::GetAvailableSlot2Weapons() {
    return {
        { 201, "Shotgun",      "Shotgun", "近距離で多粒の弾を拡散射撃。大ダメージ", 6, 18 },
        { 202, "Sniper Rifle", "Sniper",  "長距離高精度射撃。貫通特大ダメージ", 5, 80 },
        { 203, "Assault Rifle","Assault", "連射性能に優れ中距離戦で安定した威力を発揮", 30, 20 }
    };
}
