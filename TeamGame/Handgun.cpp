#include "Handgun.h"
#include "Bullet.h"

Handgun::Handgun() : Weapon("Handgun", 15) // 15フレームに1回発射可能
{
}

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (!CanFire())
        return;

    new Bullet(pos.x, pos.y, dir, 15.0f); // まっすぐ速い弾
    ResetCoolTime();
}
