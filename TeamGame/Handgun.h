#pragma once
#include "Weapon.h"

class Handgun : public Weapon
{
  public:
    Handgun();
    void Fire(const Vector2 &pos, const Vector2 &dir) override;
};
