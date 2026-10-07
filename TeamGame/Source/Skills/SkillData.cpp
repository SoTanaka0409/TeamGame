#include "SkillData.h"
#include <fstream>
#include <sstream>
#include <iostream>

SkillDataManager::SkillDataManager()
{
}

SkillDataManager& SkillDataManager::GetInstance()
{
    static SkillDataManager instance;
    return instance;
}

SkillMajorTag SkillDataManager::ParseMajorTag(const std::string& str) const
{
    if (str == "Debuff") return SkillMajorTag::Debuff;
    if (str == "Trap") return SkillMajorTag::Trap;
    return SkillMajorTag::StatusBuff;
}

SkillMinorTag SkillDataManager::ParseMinorTag(const std::string& str) const
{
    if (str == "Heal") return SkillMinorTag::Heal;
    if (str == "AttackUp") return SkillMinorTag::AttackUp;
    if (str == "SpeedUp") return SkillMinorTag::SpeedUp;
    if (str == "Blind") return SkillMinorTag::Blind;
    if (str == "Stun") return SkillMinorTag::Stun;
    return SkillMinorTag::None;
}

bool SkillDataManager::LoadFromCSV(const std::string& filePath)
{
    std::vector<std::string> searchPaths = {
        "Resource/" + filePath,
        "Source/Skills/" + filePath,
        filePath
    };

    std::ifstream file;
    for (const auto& path : searchPaths)
    {
        file.open(path);
        if (file.is_open()) break;
    }

    if (!file.is_open()) return false;

    std::string line;
    if (!std::getline(file, line)) return false; // Skip header line

    skillDatabase.clear();

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        SkillData data;

        try {
            if (!std::getline(ss, cell, ',')) continue; data.id = std::stoi(cell);
            if (!std::getline(ss, cell, ',')) continue; data.name = cell;
            if (!std::getline(ss, cell, ',')) continue; data.description = cell;
            if (!std::getline(ss, cell, ',')) continue; data.imagePath = cell;
            if (!std::getline(ss, cell, ',')) continue; data.majorTag = ParseMajorTag(cell);
            if (!std::getline(ss, cell, ',')) continue; data.minorTag = ParseMinorTag(cell);
            if (!std::getline(ss, cell, ',')) continue; data.effectValue = std::stof(cell);
            if (!std::getline(ss, cell, ',')) continue; data.duration = std::stoi(cell);
            if (!std::getline(ss, cell, ',')) continue; data.coolTime = std::stoi(cell);

            skillDatabase.push_back(data);
        } catch (...) {
            // Ignore invalid lines
        }
    }
    file.close();
    return true;
}

const SkillData* SkillDataManager::GetSkillData(int id) const
{
    for (const auto& skill : skillDatabase) {
        if (skill.id == id) return &skill;
    }
    return nullptr;
}
