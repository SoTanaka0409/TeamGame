#pragma once
#include "Weapon.h"

class Shotgun : public Weapon
{
  public:
    Shotgun();
    void Fire(const Vector2 &pos, const Vector2 &dir) override;
};
