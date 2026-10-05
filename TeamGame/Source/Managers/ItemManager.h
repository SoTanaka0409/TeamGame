#pragma once
#include <string>
#include <unordered_map>
#include <vector>

enum class ItemType;

/**
 * @brief CSV(items.csv)から読み込まれるアイテム定義データ
 */
struct ItemData
{
    std::string name;
    std::string typeName;
    int amount;
    float radius;
    std::string soundEffect;
    std::string description;
};

/**
 * @brief マップ上のアイテムパラメータをCSVから管理するシングルトンクラス
 */
class ItemManager
{
private:
    std::unordered_map<std::string, ItemData> itemDatabase;

    ItemManager() = default;
    ~ItemManager() = default;

public:
    static ItemManager& GetInstance()
    {
        static ItemManager instance;
        return instance;
    }

    ItemManager(const ItemManager&) = delete;
    ItemManager& operator=(const ItemManager&) = delete;

    /**
     * @brief CSVからアイテム定義データをロードする
     * @param filePath CSVファイルパス
     * @return bool 成功時true
     */
    bool LoadFromCSV(const std::string& filePath);

    /**
     * @brief 名前でアイテムデータを取得する
     * @param name アイテム名（例: "Health", "Ammo"）
     * @return const ItemData* アイテムデータへのポインタ（無ければnullptr）
     */
    const ItemData* GetItemData(const std::string& name) const;
};
