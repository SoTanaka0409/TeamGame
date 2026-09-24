#include "WeaponManager.h"
#include "DxLib.h"
#include <fstream>
#include <sstream>

void WeaponManager::LoadFromCSV(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        printfDx("Failed to load %s\n", filePath.c_str());
        return;
    }

    std::string line;
    // ヘッダー行をスキップ
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        WeaponData data;

        // Name, Range, SpreadAngle, BulletRadius, BulletSpeed, MaxAmmo, FireInterval, ReloadTime, ModelPath, UIImagePath
        std::getline(ss, data.name, ',');
        
        std::getline(ss, token, ','); data.range = std::stof(token);
        std::getline(ss, token, ','); data.spreadAngle = std::stof(token);
        std::getline(ss, token, ','); data.bulletRadius = std::stof(token);
        std::getline(ss, token, ','); data.bulletSpeed = std::stof(token);
        std::getline(ss, token, ','); data.maxAmmo = std::stoi(token);
        std::getline(ss, token, ','); data.fireInterval = std::stoi(token);
        std::getline(ss, token, ','); data.reloadTime = std::stoi(token);
        
        std::getline(ss, data.modelPath, ',');
        std::getline(ss, data.uiImagePath, ',');
        
        // Trim potential carriage return on last element
        if (!data.uiImagePath.empty() && data.uiImagePath.back() == '\r') {
            data.uiImagePath.pop_back();
        }

        // 画像の読み込み (もしパスがあれば)
        if (!data.uiImagePath.empty() && data.uiImagePath != "NONE")
        {
            data.uiImageHandle = LoadGraph(data.uiImagePath.c_str());
        }

        weaponDatabase[data.name] = data;
    }
    file.close();
}

const WeaponData* WeaponManager::GetWeaponData(const std::string& weaponName) const
{
    auto it = weaponDatabase.find(weaponName);
    if (it != weaponDatabase.end())
    {
        return &it->second;
    }
    return nullptr;
}

void WeaponManager::Cleanup()
{
    for (auto& pair : weaponDatabase)
    {
        if (pair.second.uiImageHandle != -1)
        {
            DeleteGraph(pair.second.uiImageHandle);
            pair.second.uiImageHandle = -1;
        }
    }
    weaponDatabase.clear();
}
