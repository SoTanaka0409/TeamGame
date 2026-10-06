#include "CharacterManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool CharacterManager::LoadFromCSV(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        // ルートに無い場合は Resource/ パスを試す
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

    characterDataMap.clear();

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        CharacterData data;

        // Type
        if (!std::getline(ss, token, ',')) continue;
        data.type = token;

        // MaxHp
        if (std::getline(ss, token, ',')) data.maxHp = std::stoi(token);

        // MoveSpeed
        if (std::getline(ss, token, ',')) data.moveSpeed = std::stof(token);

        // AttackPower
        if (std::getline(ss, token, ',')) data.attackPower = std::stoi(token);

        // ColliderRadius
        if (std::getline(ss, token, ',')) data.colliderRadius = std::stof(token);

        // SightRangeCells
        if (std::getline(ss, token, ',')) data.sightRangeCells = std::stof(token);

        // EffectiveRangeCells
        if (std::getline(ss, token, ',')) data.effectiveRangeCells = std::stof(token);

        // ShootCooldown
        if (std::getline(ss, token, ',')) data.shootCooldown = std::stoi(token);

        // AimDelayFrames
        if (std::getline(ss, token, ',')) data.aimDelayFrames = std::stoi(token);

        // RespawnTimeFrames
        if (std::getline(ss, token, ',')) data.respawnTimeFrames = std::stoi(token);

        characterDataMap[data.type] = data;
    }

    file.close();
    return true;
}

const CharacterData* CharacterManager::GetCharacterData(const std::string& type) const
{
    auto it = characterDataMap.find(type);
    if (it != characterDataMap.end())
    {
        return &it->second;
    }
    return nullptr;
}
