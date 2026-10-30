#include "WeaponManager.h"
#include "DxLib.h"
#include <fstream>
#include <sstream>
#include <algorithm>

void WeaponManager::InitDefaults()
{
    // =========================================================================
    // 【武器バランスのデフォルト標準値】 (CSVが万が一見つからない場合のセーフティ)
    // =========================================================================
    // 1. Handgun: 3発キル(12*3=36), 装弾数12, 連射16F(~3.8発/秒), リロード45F(0.75秒), 高精度(1.5度)
    weaponDatabase["Handgun"] = {
        "Handgun", 260.0f, 1.5f, 5.0f, 28.0f, 12, 16, 45, 12, 1, "Models/handgun.x", "NONE", -1
    };

    // 2. Shotgun: 至近距離6ペレット*5=30ダメージ, 装弾数6, 連射50F(~0.8秒), リロード80F, 散弾18度
    weaponDatabase["Shotgun"] = {
        "Shotgun", 150.0f, 18.0f, 4.5f, 24.0f, 6, 50, 80, 5, 6, "Models/shotgun.x", "NONE", -1
    };

    // 3. SniperRifle: 一撃必殺(36ダメージ), 貫通弾, 装弾数5, 連射65F(1.1秒), リロード85F, 超長射程500, 超高速弾55
    weaponDatabase["SniperRifle"] = {
        "SniperRifle", 500.0f, 0.0f, 7.0f, 55.0f, 5, 65, 85, 36, 1, "Models/sniper.x", "NONE", -1
    };

    // 4. SubmachineGun: フルオート連射(5F/秒間12発), 1発3ダメージ(11発キル), 装弾数28, リロード60F(1.0秒)
    weaponDatabase["SubmachineGun"] = {
        "SubmachineGun", 200.0f, 4.5f, 3.5f, 22.0f, 28, 5, 60, 3, 1, "Models/smg.x", "NONE", -1
    };
}

void WeaponManager::LoadFromCSV(const std::string& filePath)
{
    std::vector<std::string> searchPaths = {
        "Resource/CSV/" + filePath,
        "Resource/" + filePath,
        "TeamGame/Resource/CSV/" + filePath,
        "TeamGame/Resource/" + filePath,
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
        // CSVが存在しない場合でも InitDefaults() で設定されたバランスが有効
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        // コメント行 (#) やヘッダー行 (Name,Range...) をスキップ
        if (line[0] == '#' || line.find("Name,Range") != std::string::npos) continue;

        std::stringstream ss(line);
        std::string token;
        WeaponData data;

        // Name, Range, SpreadAngle, BulletRadius, BulletSpeed, MaxAmmo, FireInterval, ReloadTime, Damage, PelletCount, ModelPath, UIImagePath
        if (!std::getline(ss, data.name, ',')) continue;
        if (data.name.empty()) continue;
        
        try
        {
            if (std::getline(ss, token, ',')) data.range = std::stof(token);
            if (std::getline(ss, token, ',')) data.spreadAngle = std::stof(token);
            if (std::getline(ss, token, ',')) data.bulletRadius = std::stof(token);
            if (std::getline(ss, token, ',')) data.bulletSpeed = std::stof(token);
            if (std::getline(ss, token, ',')) data.maxAmmo = std::stoi(token);
            if (std::getline(ss, token, ',')) data.fireInterval = std::stoi(token);
            if (std::getline(ss, token, ',')) data.reloadTime = std::stoi(token);
            if (std::getline(ss, token, ',')) data.damage = std::stoi(token);
            if (std::getline(ss, token, ',')) data.pelletCount = std::stoi(token);
            
            std::getline(ss, data.modelPath, ',');
            std::getline(ss, data.uiImagePath, ',');
            
            if (!data.uiImagePath.empty() && data.uiImagePath.back() == '\r') {
                data.uiImagePath.pop_back();
            }

            if (!data.uiImagePath.empty() && data.uiImagePath != "NONE")
            {
                data.uiImageHandle = LoadGraph(("Resource/" + data.uiImagePath).c_str());
                if (data.uiImageHandle == -1)
                {
                    data.uiImageHandle = LoadGraph(data.uiImagePath.c_str());
                }
            }

            weaponDatabase[data.name] = data;
        }
        catch (...)
        {
            // パース失敗した行はスキップ
        }
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
    InitDefaults();
}
