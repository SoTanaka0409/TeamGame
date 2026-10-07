#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief キャラクターのパラメータ定義構造体 (CSV管理用)
 */
struct CharacterData
{
    int id = 1;                     ///< キャラクターID (1, 2, 3...)
    std::string name;               ///< キャラクター表示名 (例: Character 1)
    std::string type = "Assault";   ///< タイプ分類名
    int maxHp = 100;                ///< 最大体力
    float moveSpeed = 4.2f;         ///< 移動速度
    int attackPower = 20;           ///< 攻撃力
    int skillId = 1;                ///< 所持するスキルのID (skills.csvのIDに対応)
    std::string description;        ///< 特徴・説明文

    // 互換パラメータ
    float colliderRadius = 25.0f;
    float sightRangeCells = 14.0f;
    float effectiveRangeCells = 10.0f;
    int shootCooldown = 75;
    int aimDelayFrames = 25;
    int respawnTimeFrames = 180;
};

/**
 * @brief CSVから全キャラクターパラメータをロード・一括管理するシングルトンマネージャー
 */
class CharacterManager
{
private:
    std::unordered_map<std::string, CharacterData> characterDataMap;
    std::unordered_map<int, CharacterData> characterDataIdMap;
    std::vector<CharacterData> characterList;

    CharacterManager() = default;
    ~CharacterManager() = default;

public:
    static CharacterManager& GetInstance()
    {
        static CharacterManager instance;
        return instance;
    }

    CharacterManager(const CharacterManager&) = delete;
    CharacterManager& operator=(const CharacterManager&) = delete;

    /**
     * @brief CSVファイルからキャラクターパラメータを読み込む
     * @param filePath CSVファイルのパス
     * @return bool 成功時true
     */
    bool LoadFromCSV(const std::string& filePath);

    /**
     * @brief キャラクターID (1~N) でデータを取得
     */
    const CharacterData* GetCharacterDataById(int id) const;

    /**
     * @brief タイプ名 (文字列) でデータを取得 (旧互換用)
     */
    const CharacterData* GetCharacterData(const std::string& type) const;

    /**
     * @brief ロードされている全キャラクターデータの一覧を取得
     */
    const std::vector<CharacterData>& GetAllCharacters() const { return characterList; }
};
