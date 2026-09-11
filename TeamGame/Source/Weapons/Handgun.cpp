#include "Handgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"

Handgun::Handgun() : Weapon("Handgun") {}

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            // Create bullet using CSV data
            Bullet *bullet = new Bullet(pos.x, pos.y, dir, data->bulletSpeed, data->range, data->bulletRadius);
            scene->GetObjectManager()->AddObject(bullet);
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
