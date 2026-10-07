#pragma once
#include "Vector2.h"
#include "../Managers/WeaponManager.h"
#include <string>

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
    explicit Weapon(const std::string &name);
    virtual ~Weapon() = default;

    virtual void Update();
    virtual void Fire(const Vector2 &pos, const Vector2 &dir, int teamId = 0, bool isMoving = false) = 0;
    virtual void Reload();

    bool CanFire() const;
    void UseAmmo(int amount = 1);
    void AddAmmo(int amount);
    void ResetCoolTime();

    std::string GetName() const { return weaponName; }
    int GetCurrentAmmo() const { return currentAmmo; }
    int GetMaxAmmo() const;
    bool IsReloading() const { return isReloading; }
    const WeaponData* GetData() const { return data; }
    virtual float GetMoveSpreadPenalty() const { return 0.0f; }
};
