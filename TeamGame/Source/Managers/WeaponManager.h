#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct WeaponData
{
    std::string name;
    float range;          // 飛ぶ距離
    float spreadAngle;    // ブレの大きさ(度)
    float bulletRadius;   // 弾のサイズ
    float bulletSpeed;    // 弾の速さ
    int maxAmmo;          // 装弾数
    int fireInterval;     // 撃つ間隔(フレーム)
    int reloadTime;       // リロード時間(フレーム)
    std::string modelPath; // 3Dスキン等のパス
    std::string uiImagePath; // 画面端のUI画像パス
    int uiImageHandle = -1;  // ロードした画像のハンドル
};

class WeaponManager
{
private:
    std::unordered_map<std::string, WeaponData> weaponDatabase;
    WeaponManager() {}
    ~WeaponManager() {}

public:
    static WeaponManager& GetInstance()
    {
        static WeaponManager instance;
        return instance;
    }

    WeaponManager(const WeaponManager&) = delete;
    WeaponManager& operator=(const WeaponManager&) = delete;

    void LoadFromCSV(const std::string& filePath);
    const WeaponData* GetWeaponData(const std::string& weaponName) const;
    void Cleanup();
};
