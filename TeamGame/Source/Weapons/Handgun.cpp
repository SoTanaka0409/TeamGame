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

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir, int teamId)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            // Create bullet using CSV data
            new Bullet(pos.x, pos.y, dir, data->bulletSpeed, data->range, data->bulletRadius);
            if (scene->GetEffectManager())
            {
                float angle = std::atan2(dir.y, dir.x);
                scene->GetEffectManager()->AddMuzzleFlashEffect(pos.x + dir.x * 25.0f, pos.y + dir.y * 25.0f, angle, 16.0f);
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
