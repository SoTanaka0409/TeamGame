#include "Shotgun.h"
#include "Bullet.h"
#include "Enemy.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include <cmath>

Shotgun::Shotgun() : Weapon("Shotgun", 45)
{
}

static void NotifyEnemiesOfGunshot(const Vector2 &pos)
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            Enemy *enemy = dynamic_cast<Enemy *>(obj);
            if (enemy && enemy->IsActive())
            {
                enemy->OnHearGunshot(pos);
            }
        }
    }
}

void Shotgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (!CanFire())
        return;

    float baseAngle = std::atan2(dir.y, dir.x);
    int bulletCount = 15;
    float spreadRange = 1.25f;

    for (int i = 0; i < bulletCount; ++i)
    {
        float angleOffset =
            -spreadRange + (spreadRange * 2.0f / (bulletCount - 1)) * i;
        float angle = baseAngle + angleOffset;
        Vector2 spreadDir(std::cos(angle), std::sin(angle));
        new Bullet(pos.x, pos.y, spreadDir, 12.0f);
    }

    NotifyEnemiesOfGunshot(pos);
    ResetCoolTime();
}
