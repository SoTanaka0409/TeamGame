#pragma once

#include <string>
#include <vector>

/**
 * @brief キャラクター定義データ
 */
struct CharacterData {
    int id = 1;                         // キャラクターID
    std::string name;                   // キャラクター名
    std::string typeName;               // タイトル・タイプ
    std::string description;            // 特徴・説明
    int maxHp = 100;                    // 最大HP
    float moveSpeed = 4.0f;             // 移動速度
    unsigned int themeColor = 0xFFFFFF; // UI上のテーマカラー
};

/**
 * @brief 武器選択用データ
 */
struct WeaponSelectData {
    int id = 0;                         // 武器ID
    std::string name;                   // 武器名
    std::string category;               // カテゴリ (例: Handgun, Shotgun, Sniper)
    std::string description;            // 特長・性能説明
    int maxAmmo = 10;                   // 装弾数
    int damage = 10;                    // 威力
};

/**
 * @brief プレイヤーが選択したキャラクターおよび武器データ
 */
struct PlayerSelection {
    CharacterData selectedCharacter;    // 選択したキャラクター
    WeaponSelectData slot1Weapon;       // スロット1 (ハンドガン固定)
    WeaponSelectData slot2Weapon;       // スロット2 (自由選択武器)
};

/**
 * @brief シーン間での選択データ受け渡し・管理クラス (Singleton)
 */
class PlayerSelectionManager {
public:
    static PlayerSelectionManager& GetInstance();

    void SetSelectedCharacter(const CharacterData& chara) { m_selection.selectedCharacter = chara; }
    void SetSlot1Weapon(const WeaponSelectData& weapon) { m_selection.slot1Weapon = weapon; }
    void SetSlot2Weapon(const WeaponSelectData& weapon) { m_selection.slot2Weapon = weapon; }

    const PlayerSelection& GetSelection() const { return m_selection; }
    PlayerSelection& GetSelection() { return m_selection; }

    /**
     * @brief 利用可能な全キャラクターリストを取得 (1~5番)
     */
    static std::vector<CharacterData> GetAvailableCharacters();

    /**
     * @brief スロット1用の固定武器 (ハンドガン) を取得
     */
    static WeaponSelectData GetFixedSlot1Weapon();

    /**
     * @brief スロット2用の選択可能武器リストを取得
     */
    static std::vector<WeaponSelectData> GetAvailableSlot2Weapons();

private:
    PlayerSelectionManager();
    PlayerSelection m_selection;
};
