#include "GameBalanceManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool GameBalanceManager::LoadFromCSV(const std::string& filePath)
{
    std::vector<std::string> searchPaths = {
        filePath,
        "Resource/CSV/" + filePath,
        "Resource/" + filePath,
        "TeamGame/Resource/CSV/" + filePath,
        "TeamGame/Resource/" + filePath
    };
    std::ifstream file;
    for (const auto& path : searchPaths)
    {
        file.open(path);
        if (file.is_open()) break;
    }

    if (!file.is_open())
    {
        return false;
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
        std::getline(ss, desc, ',');

        if (key.empty()) continue;

        try
        {
            if (valStr.find('.') != std::string::npos)
            {
                floatValues[key] = std::stof(valStr);
            }
            else
            {
                intValues[key] = std::stoi(valStr);
            }
        }
        catch (...)
        {
            // パースエラーは無視してスキップ
        }
    }

    file.close();
    return true;
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

int GameBalanceManager::GetInt(const std::string& key, int defaultValue) const
{
    auto it = intValues.find(key);
    if (it != intValues.end())
    {
        return it->second;
    }
    return defaultValue;
}
