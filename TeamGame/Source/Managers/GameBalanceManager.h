#pragma once
#include <string>
#include <unordered_map>

/**
 * @brief CSV(game_balance.csv)からゲーム全般のルール・定数を一括管理するクラス
 */
class GameBalanceManager
{
private:
    std::unordered_map<std::string, float> floatValues;
    std::unordered_map<std::string, int> intValues;

    GameBalanceManager() = default;
    ~GameBalanceManager() = default;

public:
    static GameBalanceManager& GetInstance()
    {
        static GameBalanceManager instance;
        return instance;
    }

    GameBalanceManager(const GameBalanceManager&) = delete;
    GameBalanceManager& operator=(const GameBalanceManager&) = delete;

    /**
     * @brief CSVファイルから定数テーブルをロードする
     * @param filePath CSVファイルのパス
     * @return bool 成功時true
     */
    bool LoadFromCSV(const std::string& filePath);

    /**
     * @brief 整数値パラメータの取得
     * @param key キー名
     * @param defaultValue 見つからない場合のデフォルト値
     * @return int パラメータ値
     */
    int GetInt(const std::string& key, int defaultValue = 0) const;

    /**
     * @brief 浮動小数点数パラメータの取得
     * @param key キー名
     * @param defaultValue 見つからない場合のデフォルト値
     * @return float パラメータ値
     */
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
};
