#include "Shotgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include <cmath>


#include "Enemy.h"

static void NotifyEnemiesOfGunshot(const Vector2 &pos, int shooterTeamId)
{
    auto scene = SceneManager::GetInstance().GetCurrentScene();
    if (scene && scene->GetObjectManager())
    {
        for (auto obj : scene->GetObjectManager()->GetObjects())
        {
            Enemy *enemy = dynamic_cast<Enemy *>(obj);
            if (enemy && enemy->IsActive())
            {
                enemy->OnHearGunshot(pos, 1200.0f, shooterTeamId);
            }
        }
    }
}

Shotgun::Shotgun() : Weapon("Shotgun") {}

void Shotgun::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, bool isMoving)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            int pelletCount = (data && data->pelletCount > 0) ? data->pelletCount : 5;
            float spreadMultiplier = isMoving ? 1.6f : 1.0f;
            float spreadRad = data->spreadAngle * spreadMultiplier * (3.14159f / 180.0f);
            float baseAngle = std::atan2(dir.y, dir.x);

            for (int i = 0; i < pelletCount; i++)
            {
                // -spread/2 to +spread/2
                float angleOffset = (pelletCount > 1) ? (-spreadRad / 2.0f + (spreadRad / (pelletCount - 1)) * i) : 0.0f;
                if (isMoving)
                {
                    // 移動時は各ペレットに小さなランダムブレを上乗せ
                    angleOffset += ((std::rand() % 1000) / 1000.0f - 0.5f) * 0.08f;
                }
                float finalAngle = baseAngle + angleOffset;
                Vector2 fireDir(std::cos(finalAngle), std::sin(finalAngle));

                new Bullet(pos.x, pos.y, fireDir, data->bulletSpeed, data->range, data->bulletRadius, teamId, data->damage);
            }
            if (scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddMuzzleFlashEffect(pos.x + dir.x * 25.0f, pos.y + dir.y * 25.0f, baseAngle, 24.0f);
            }
            NotifyEnemiesOfGunshot(pos, teamId);
            ResetCoolTime();
            UseAmmo(1);
        }
    }
    else if (currentAmmo <= 0 && !isReloading)
    {
        Reload();
    }
}
