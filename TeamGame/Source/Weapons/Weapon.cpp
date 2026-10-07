#include "Weapon.h"
#include "../Managers/WeaponManager.h"

Weapon::Weapon(const std::string &name)
    : weaponName(name), coolTimeTimer(0), isReloading(false), reloadTimer(0)
{
    data = WeaponManager::GetInstance().GetWeaponData(name);
    if (data) {
        currentAmmo = data->maxAmmo;
    } else {
        currentAmmo = 0;
    }
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
