#include "Handgun.h"
#include "Bullet.h"
#include "Enemy.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"

Handgun::Handgun() : Weapon("Handgun", 15)
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

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (!CanFire())
        return;

    new Bullet(pos.x, pos.y, dir, 15.0f);
    NotifyEnemiesOfGunshot(pos);
    ResetCoolTime();
}
