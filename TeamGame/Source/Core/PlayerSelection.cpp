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
        { 1, "Character 1", "Assault", "Standard balanced character (Green)", 100, 4.2f, 0x00FF88, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png" },
        { 2, "Character 2", "Speed", "High mobility", 80, 5.2f, 0x00A2FF, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png" },
        { 3, "Character 3", "Heavy", "Tank role with high HP", 150, 3.4f, 0xFF4444, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png" },
        { 4, "Character 4", "Sniper", "Long range specialist", 90, 4.0f, 0xFFBB00, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png" },
        { 5, "Character 5", "Tactical", "High turn rate with skills", 110, 4.5f, 0xAA44FF, "Resource/GreenCharacter/character.mv1", "Resource/GreenCharacter/preview.png" }
    };
}

WeaponSelectData PlayerSelectionManager::GetFixedSlot1Weapon() {
    return { 101, "Handgun", "Handgun", "Standard Handgun", 12, 25 };
}

std::vector<WeaponSelectData> PlayerSelectionManager::GetAvailableSlot2Weapons() {
    return {
        { 201, "Shotgun",      "Shotgun", "Close range spread", 6, 18 },
        { 202, "Sniper Rifle", "Sniper",  "Long range pierce", 5, 80 },
        { 203, "Assault Rifle","Assault", "Medium range automatic", 30, 20 }
    };
}
