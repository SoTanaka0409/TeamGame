#include "GameBalanceManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool GameBalanceManager::LoadFromCSV(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::string backupPath = "Resource/" + filePath;
        file.open(backupPath);
        if (!file.is_open())
        {
            return false;
        }
    }

    std::string line;
    // ヘッダー行を読み飛ばす
    if (!std::getline(file, line))
    {
        return false;
    }

    floatValues.clear();
    intValues.clear();

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string key, valStr, desc;

        if (!std::getline(ss, key, ',')) continue;
        if (!std::getline(ss, valStr, ',')) continue;

        try {
            float fVal = std::stof(valStr);
            int iVal = std::stoi(valStr);
            floatValues[key] = fVal;
            intValues[key] = iVal;
        }
        catch (...) {
            // パース失敗時はスキップ
        }
    }

    file.close();
    return true;
}

int GameBalanceManager::GetInt(const std::string& key, int defaultValue) const
{
    auto it = intValues.find(key);
    if (it != intValues.end())
    {
        return it->second;
    }
    return defaultValue;
}

float GameBalanceManager::GetFloat(const std::string& key, float defaultValue) const
{
    auto it = floatValues.find(key);
    if (it != floatValues.end())
    {
        return it->second;
    }
    return defaultValue;
}
