#pragma once
#include "Vector2.h"
#include "../Managers/WeaponManager.h"
#include "BulletVisualConfig.h"
#include <string>

/**
 * @brief 武器の基底クラス
 * @details 射撃、リロード、クールタイム、残弾数、および銃身の描画（GunBarrelVisualConfig）を管理します。
 */
class Weapon
{
protected:
    int coolTimeTimer;
    std::string weaponName;
    const WeaponData* data;
    WeaponData customWeaponData;  ///< 動的・派生生成された武器パラメータ
    bool hasCustomData = false;   ///< 動的パラメータを使用しているか
    int currentAmmo;
    bool isReloading;
    int reloadTimer;
    GunBarrelVisualConfig barrelVisual; ///< 銃身（バレル）の描画アセット設定

public:
    explicit Weapon(const std::string &name);
    Weapon(const std::string &name, const WeaponData& customData, const GunBarrelVisualConfig& visual = GunBarrelVisualConfig());
    virtual ~Weapon() = default;

    virtual void Update();
    virtual void Fire(const Vector2 &pos, const Vector2 &dir, int teamId = 0, bool isMoving = false, const std::string& shooterName = "", bool isPanic = false) = 0;
    virtual void Reload();

    /**
     * @brief 銃身（ガンバレル）の描画処理
     * @param screenX 画面上の基準X座標
     * @param screenY 画面上の基準Y座標
     * @param facingDir 照準・構え方向ベクトル
     */
    virtual void DrawBarrel(float screenX, float screenY, const Vector2 &facingDir);

    bool CanFire() const;
    void UseAmmo(int amount = 1);
    void AddAmmo(int amount);
    void ResetCoolTime();

    std::string GetName() const { return weaponName; }
    void SetName(const std::string& name) { weaponName = name; }
    int GetCurrentAmmo() const { return currentAmmo; }
    int GetMaxAmmo() const;
    bool IsReloading() const { return isReloading; }
    const WeaponData* GetData() const { return data; }
    virtual float GetMoveSpreadPenalty() const { return 0.0f; }

    const GunBarrelVisualConfig& GetBarrelVisual() const { return barrelVisual; }
    void SetBarrelVisual(const GunBarrelVisualConfig& visual) { barrelVisual = visual; }
};
