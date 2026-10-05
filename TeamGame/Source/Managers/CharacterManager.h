#pragma once
#include <string>
#include <unordered_map>

/**
 * @brief キャラクターの初期パラメータ構造体
 */
struct CharacterData
{
    std::string type;
    int maxHp = 10;
    float moveSpeed = 0.75f;
    int attackPower = 1;
    float colliderRadius = 25.0f;
    float sightRangeCells = 14.0f;
    float effectiveRangeCells = 10.0f;
    int shootCooldown = 75;
    int aimDelayFrames = 25;
    int respawnTimeFrames = 180;
};

/**
 * @brief CSVからキャラクターパラメータをロード・管理するシングルトンクラス
 */
class CharacterManager
{
private:
    std::unordered_map<std::string, CharacterData> characterDataMap;

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
     * @return bool 成功した場合はtrue
     */
    bool LoadFromCSV(const std::string& filePath);

    /**
     * @brief 指定したタイプのキャラクターデータの取得
     * @param type キャラクタータイプ名 (Player, EnemyBot, AllyBot 等)
     * @return const CharacterData* データへのポインタ（見つからない場合はnullptr）
     */
    const CharacterData* GetCharacterData(const std::string& type) const;
};
