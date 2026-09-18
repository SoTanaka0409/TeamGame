#include <cmath>
#include <cstdlib>
#include "Handgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SoundManager.h"

#include "Enemy.h"


Handgun::Handgun() : Weapon("Handgun") {}

void Handgun::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, float additionalSpread)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            // Create bullet and add to ObjectManager
            float totalSpread = data->spreadAngle + additionalSpread;
            float halfSpreadRad = (totalSpread / 2.0f) * (3.14159265f / 180.0f);
            float randomAngle = 0.0f;
            if (halfSpreadRad > 0.0f) {
                randomAngle = (((float)std::rand() / RAND_MAX) * (halfSpreadRad * 2.0f)) - halfSpreadRad;
            }
            float currentAngle = std::atan2(dir.y, dir.x);
            float finalAngle = currentAngle + randomAngle;
            Vector2 finalDir(std::cos(finalAngle), std::sin(finalAngle));

            Bullet* bullet = new Bullet(pos.x, pos.y, finalDir, data->bulletSpeed, data->range, data->bulletRadius, teamId);
            // scene->GetObjectManager()->AddObject(bullet);
            
            // 3D銃声を鳴らす（最大聞こえる距離を1000として設定）
            SoundManager::GetInstance().Play3D("gunshot", pos, 1000.0f);

            
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
