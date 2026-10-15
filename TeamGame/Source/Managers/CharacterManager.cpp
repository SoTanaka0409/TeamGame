#include "CharacterManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool CharacterManager::LoadFromCSV(const std::string& filePath)
{
    std::vector<std::string> searchPaths = {
        filePath,
        "Resource/" + filePath,
        "Source/Characters/" + filePath
    };

    std::ifstream file;
    for (const auto& path : searchPaths)
    {
        file.open(path);
        if (file.is_open()) break;
    }

    if (!file.is_open()) return false;

    std::string line;
    // ヘッダー行を読み飛ばす
    if (!std::getline(file, line)) return false;

    characterDataMap.clear();
    characterDataIdMap.clear();
    characterList.clear();

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        CharacterData data;

        try {
            // 新形式: Id, Name, Type, MaxHp, MoveSpeed, AttackPower, SkillId, Description
            // もしくは旧形式: Type, MaxHp, MoveSpeed, AttackPower...
            std::vector<std::string> tokens;
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens.empty()) continue;

            // 数値から始まっている場合 (新形式: Idスタート)
            if (std::isdigit(static_cast<unsigned char>(tokens[0][0])))
            {
                if (tokens.size() >= 1) data.id = std::stoi(tokens[0]);
                if (tokens.size() >= 2) data.name = tokens[1];
                if (tokens.size() >= 3) data.type = tokens[2];
                if (tokens.size() >= 4) data.maxHp = std::stoi(tokens[3]);
                if (tokens.size() >= 5) data.moveSpeed = std::stof(tokens[4]);
                if (tokens.size() >= 6) data.attackPower = std::stoi(tokens[5]);
                if (tokens.size() >= 7) data.skillId = std::stoi(tokens[6]);
                if (tokens.size() >= 8) data.description = tokens[7];
            }
            else // 旧形式 (Typeスタート)
            {
                data.type = tokens[0];
                data.name = tokens[0];
                if (tokens.size() >= 2) data.maxHp = std::stoi(tokens[1]);
                if (tokens.size() >= 3) data.moveSpeed = std::stof(tokens[2]);
                if (tokens.size() >= 4) data.attackPower = std::stoi(tokens[3]);
                if (tokens.size() >= 5) data.colliderRadius = std::stof(tokens[4]);
                if (tokens.size() >= 6) data.sightRangeCells = std::stof(tokens[5]);
                if (tokens.size() >= 7) data.effectiveRangeCells = std::stof(tokens[6]);
                if (tokens.size() >= 8) data.shootCooldown = std::stoi(tokens[7]);
                if (tokens.size() >= 9) data.aimDelayFrames = std::stoi(tokens[8]);
                if (tokens.size() >= 10) data.respawnTimeFrames = std::stoi(tokens[9]);
            }

            characterDataMap[data.type] = data;
            characterDataIdMap[data.id] = data;
            characterList.push_back(data);

        } catch (...) {
            // パース失敗行は安全にスキップ
        }
    }

    file.close();
    return true;
}

const CharacterData* CharacterManager::GetCharacterDataById(int id) const
{
    auto it = characterDataIdMap.find(id);
    if (it != characterDataIdMap.end())
    {
        return &it->second;
    }
    return nullptr;
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
