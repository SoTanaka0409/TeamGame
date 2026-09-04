#pragma once
#include "Vector2.h"
#include <string>

class Weapon
{
  protected:
    int coolTimeTimer;
    int fireRate; // クールタイムの最大値（何フレームに1回撃てるか）
    std::string weaponName;

  public:
    Weapon(const std::string &name, int fireRate)
        : weaponName(name), fireRate(fireRate), coolTimeTimer(0)
    {
    }
    virtual ~Weapon()
    {
    }

    virtual void Update()
    {
        if (coolTimeTimer > 0)
            coolTimeTimer--;
    }

    virtual void Fire(const Vector2 &pos, const Vector2 &dir) = 0;

    bool CanFire() const
    {
        return coolTimeTimer <= 0;
    }
    void ResetCoolTime()
    {
        coolTimeTimer = fireRate;
    }

    std::string GetName() const
    {
        return weaponName;
    }
};
