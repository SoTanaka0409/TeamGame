#include "Weapon.h"
#include "../Managers/WeaponManager.h"
#include "DxLib.h"
#include <cmath>

Weapon::Weapon(const std::string &name)
    : weaponName(name), coolTimeTimer(0), isReloading(false), reloadTimer(0), hasCustomData(false)
{
    data = WeaponManager::GetInstance().GetWeaponData(name);
    if (data) {
        currentAmmo = data->maxAmmo;
    } else {
        currentAmmo = 0;
    }
}

Weapon::Weapon(const std::string &name, const WeaponData& customData, const GunBarrelVisualConfig& visual)
    : weaponName(name), coolTimeTimer(0), isReloading(false), reloadTimer(0),
      customWeaponData(customData), hasCustomData(true), barrelVisual(visual)
{
    data = &customWeaponData;
    currentAmmo = data->maxAmmo;
}

void Weapon::Update()
{
    if (isReloading) {
        if (reloadTimer > 0) reloadTimer--;
        if (reloadTimer <= 0) {
            isReloading = false;
            if (data) currentAmmo = data->maxAmmo;
        }
    }
    else {
        if (coolTimeTimer > 0)
            coolTimeTimer--;
    }
}

void Weapon::Reload()
{
    if (!isReloading && data && currentAmmo < data->maxAmmo) {
        isReloading = true;
        reloadTimer = data->reloadTime;
    }
}

void Weapon::DrawBarrel(float screenX, float screenY, const Vector2 &facingDir)
{
    if (!barrelVisual.enabled) return;

    float len = std::sqrt(facingDir.x * facingDir.x + facingDir.y * facingDir.y);
    if (len < 0.0001f) return;

    float nx = facingDir.x / len;
    float ny = facingDir.y / len;
    // 垂直（法線）ベクトル
    float px = -ny;
    float py = nx;

    float startDist = 14.0f; // キャラクター中心からの開始距離
    float endDist = startDist + barrelVisual.barrelLength;

    float sx = screenX + nx * startDist;
    float sy = screenY + ny * startDist;
    float ex = screenX + nx * endDist;
    float ey = screenY + ny * endDist;

    // 1. 銃身ベース（メインバレル）
    int bWidth = (std::max)(1, barrelVisual.barrelWidth);
    DrawLine(static_cast<int>(sx), static_cast<int>(sy),
             static_cast<int>(ex), static_cast<int>(ey),
             barrelVisual.barrelColor, bWidth);

    // 2. アクセント／LED発光ライン（銃身上部または中心の細い光）
    float accentStartDist = startDist + 4.0f;
    float accentEndDist = endDist - 6.0f;
    if (accentEndDist > accentStartDist)
    {
        float asx = screenX + nx * accentStartDist;
        float asy = screenY + ny * accentStartDist;
        float aex = screenX + nx * accentEndDist;
        float aey = screenY + ny * accentEndDist;
        DrawLine(static_cast<int>(asx), static_cast<int>(asy),
                 static_cast<int>(aex), static_cast<int>(aey),
                 barrelVisual.accentColor, 1);
    }

    // 3. マズルブレーキ／フラッシュハイダー（先端）
    if (barrelVisual.hasMuzzleBrake)
    {
        float mStartDist = endDist - 5.0f;
        float msx = screenX + nx * mStartDist;
        float msy = screenY + ny * mStartDist;
        DrawLine(static_cast<int>(msx), static_cast<int>(msy),
                 static_cast<int>(ex), static_cast<int>(ey),
                 barrelVisual.muzzleColor, bWidth + static_cast<int>(barrelVisual.muzzleWidthOffset));
    }
}

bool Weapon::CanFire() const
{
    return !isReloading && coolTimeTimer <= 0 && currentAmmo > 0;
}

void Weapon::UseAmmo(int amount)
{
    currentAmmo -= amount;
    if (currentAmmo <= 0) currentAmmo = 0;
}

void Weapon::AddAmmo(int amount)
{
    if (data) {
        currentAmmo += amount;
        if (currentAmmo > data->maxAmmo) {
            currentAmmo = data->maxAmmo;
        }
    }
}

void Weapon::ResetCoolTime()
{
    if (data) coolTimeTimer = data->fireInterval;
}

int Weapon::GetMaxAmmo() const
{
    return data ? data->maxAmmo : 0;
}
