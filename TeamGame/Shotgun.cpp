#include "Shotgun.h"
#include "Bullet.h"
#include <cmath>

Shotgun::Shotgun() : Weapon("Shotgun", 45) // 45フレームに1回発射可能
{
}

void Shotgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (!CanFire())
        return;

    // 現在向いている方向の角度を計算
    float baseAngle = std::atan2(dir.y, dir.x);

    // 扇の大きさを5倍に拡大し、弾の数も15発に増やしてド派手なショットガンにする
    int bulletCount = 15;
    float spreadRange = 1.25f; // 元の0.25の5倍の広がり（約左右71度）

    for (int i = 0; i < bulletCount; ++i)
    {
        // -spreadRange から +spreadRange までの間に均等に弾を散らす
        float angleOffset =
            -spreadRange + (spreadRange * 2.0f / (bulletCount - 1)) * i;
        float angle = baseAngle + angleOffset;
        Vector2 spreadDir(std::cos(angle), std::sin(angle));
        new Bullet(pos.x, pos.y, spreadDir, 12.0f);
    }

    ResetCoolTime();
}
