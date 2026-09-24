#pragma once
#include "Vector2.h"
#include <string>
#include "WeaponManager.h"

class Weapon
{
  protected:
    int coolTimeTimer;
    std::string weaponName;
    const WeaponData* data;
    int currentAmmo;
    bool isReloading;
    int reloadTimer;

  public:
    Weapon(const std::string &name)
        : weaponName(name), coolTimeTimer(0), isReloading(false), reloadTimer(0)
    {
        data = WeaponManager::GetInstance().GetWeaponData(name);
        if (data) {
            currentAmmo = data->maxAmmo;
        } else {
            currentAmmo = 0;
        }
    }
    virtual ~Weapon() {}

    virtual void Update()
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

    virtual void Fire(const Vector2 &pos, const Vector2 &dir, int teamId = 0, float additionalSpread = 0.0f) = 0;
    
    virtual void Reload()
    {
        if (!isReloading && data && currentAmmo < data->maxAmmo) {
            isReloading = true;
            reloadTimer = data->reloadTime;
        }
    }

    bool CanFire() const
    {
        return !isReloading && coolTimeTimer <= 0 && currentAmmo > 0;
    }
    
    void UseAmmo(int amount = 1)
    {
        currentAmmo -= amount;
        if (currentAmmo <= 0) currentAmmo = 0;
    }

    void ResetCoolTime()
    {
        if (data) coolTimeTimer = data->fireInterval;
    }

    std::string GetName() const { return weaponName; }
    int GetCurrentAmmo() const { return currentAmmo; }
    int GetMaxAmmo() const { return data ? data->maxAmmo : 0; }
    bool IsReloading() const { return isReloading; }
    const WeaponData* GetData() const { return data; }
};
