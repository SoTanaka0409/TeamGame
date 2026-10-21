#pragma once
#include "Weapon.h"

class Shotgun : public Weapon
{
  public:
    Shotgun();
    void Fire(const Vector2 &pos, const Vector2 &dir, int teamId = 0, bool isMoving = false, const std::string& shooterName = "", bool isPanic = false) override;
};
