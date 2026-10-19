#include "ItemManager.h"
#include <fstream>
#include <sstream>
#include <vector>

bool ItemManager::LoadFromCSV(const std::string& filePath)
{
    std::vector<std::string> searchPaths = {
        "Resource/" + filePath,
        filePath
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
    // ヘッダー行をスキップ
    if (!std::getline(file, line))
    {
        file.close();
        return false;
    }

    itemDatabase.clear();

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        ItemData data;

        try {
            // Name,Type,Amount,Radius,SoundEffect,Description
            if (!std::getline(ss, data.name, ',')) continue;
            if (!std::getline(ss, data.typeName, ',')) continue;
            if (!std::getline(ss, token, ',')) continue; data.amount = std::stoi(token);
            if (!std::getline(ss, token, ',')) continue; data.radius = std::stof(token);
            if (!std::getline(ss, data.soundEffect, ',')) continue;
            if (!std::getline(ss, data.description, ',')) continue;

            if (!data.description.empty() && data.description.back() == '\r') {
                data.description.pop_back();
            }

            itemDatabase[data.name] = data;
            itemDatabase[data.typeName] = data;
        } catch (...) {
            // パース失敗行はスキップ
        }
    }

    file.close();
    return true;
}

const ItemData* ItemManager::GetItemData(const std::string& name) const
{
    auto it = itemDatabase.find(name);
    if (it != itemDatabase.end())
    {
        return &it->second;
    }
    return nullptr;
}
