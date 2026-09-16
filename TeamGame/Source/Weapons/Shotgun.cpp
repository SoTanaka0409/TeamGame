#include "Shotgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include <cmath>

#include "Enemy.h"

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

Shotgun::Shotgun() : Weapon("Shotgun") {}

void Shotgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            int pelletCount = 5;
            float spreadRad = data->spreadAngle * (3.14159f / 180.0f);
            float baseAngle = std::atan2(dir.y, dir.x);

            for (int i = 0; i < pelletCount; i++)
            {
                // -spread/2 to +spread/2
                float angleOffset = (pelletCount > 1) ? (-spreadRad / 2.0f + (spreadRad / (pelletCount - 1)) * i) : 0.0f;
                float finalAngle = baseAngle + angleOffset;
                Vector2 fireDir(std::cos(finalAngle), std::sin(finalAngle));

                Bullet* bullet = new Bullet(pos.x, pos.y, fireDir, data->bulletSpeed, data->range, data->bulletRadius);
                scene->GetObjectManager()->AddObject(bullet);
            }
            
            SoundManager::GetInstance().Play3D("shotgun_fire", pos, 1200.0f);

            NotifyEnemiesOfGunshot(pos);
            ResetCoolTime();
            UseAmmo(1);
        }
    }
    else if (currentAmmo <= 0 && !isReloading)
    {
        Reload();
    }
}
