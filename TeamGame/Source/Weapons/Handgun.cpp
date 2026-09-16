#include "Handgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SoundManager.h"

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

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            // Create bullet and add to ObjectManager
            Bullet* bullet = new Bullet(pos.x, pos.y, dir, data->bulletSpeed, data->range, data->bulletRadius);
            scene->GetObjectManager()->AddObject(bullet);
            
            // 3D銃声を鳴らす（最大聞こえる距離を1000として設定）
            SoundManager::GetInstance().Play3D("gunshot", pos, 1000.0f);

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
