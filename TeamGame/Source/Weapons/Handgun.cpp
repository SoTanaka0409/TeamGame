#include "Handgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"


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

Handgun::Handgun() : Weapon("Handgun") {}

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, bool isMoving)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            // 移動状態に応じた拡散角（静止時: 約±2°, 移動時: 約±9°）
            float maxSpreadRad = isMoving ? 0.157f : 0.035f;
            float angleOffset = ((std::rand() % 1000) / 1000.0f - 0.5f) * maxSpreadRad;
            float baseAngle = std::atan2(dir.y, dir.x);
            float finalAngle = baseAngle + angleOffset;
            Vector2 finalDir(std::cos(finalAngle), std::sin(finalAngle));

            // Create bullet using CSV data & spread direction
            new Bullet(pos.x, pos.y, finalDir, data->bulletSpeed, data->range, data->bulletRadius);
            if (scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddMuzzleFlashEffect(pos.x + dir.x * 25.0f, pos.y + dir.y * 25.0f, finalAngle, 16.0f);
            }
            NotifyEnemiesOfGunshot(pos);
            ResetCoolTime();
            UseAmmo(1);
        }
    }
    else if (currentAmmo <= 0 && !isReloading)
    {
        // Auto-reload on empty
        Reload();
    }
}
