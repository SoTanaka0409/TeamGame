#include <cstdlib>
#include "Shotgun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include <cmath>

#include "Enemy.h"


/**
 * @brief Shotgunのコンストラクタ
 * @details 親クラスWeaponを"Shotgun"という名前で初期化する
 */
Shotgun::Shotgun() : Weapon("Shotgun") {}

/**
 * @brief 複数の弾を発射する実装
 * @param pos 発射位置
 * @param dir 発射方向
 * @param teamId チームID
 * @param additionalSpread 追加の拡散角度
 * @details 扇状に広がるように計算された角度で複数のBulletオブジェクトを生成し、ショットガン専用の銃声を鳴らす
 */
void Shotgun::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, float additionalSpread)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            int pelletCount = 5;
            float totalSpread = data->spreadAngle + additionalSpread;
            float spreadRad = totalSpread * (3.14159f / 180.0f);
            float baseAngle = std::atan2(dir.y, dir.x);

            for (int i = 0; i < pelletCount; i++)
            {
                float angleOffset = (pelletCount > 1) ? (-spreadRad / 2.0f + (spreadRad / (pelletCount - 1)) * i) : 0.0f;
                float jitter = (((float)std::rand() / RAND_MAX) * 0.1f) - 0.05f; // Small random jitter
                float finalAngle = baseAngle + angleOffset + jitter;
                Vector2 fireDir(std::cos(finalAngle), std::sin(finalAngle));

                Bullet* bullet = new Bullet(pos.x, pos.y, fireDir, data->bulletSpeed, data->range, data->bulletRadius, teamId);
                // scene->GetObjectManager()->AddObject(bullet);
            }
            
            SoundManager::GetInstance().Play3D("shotgun_fire", pos, 1200.0f);

            
            ResetCoolTime();
            UseAmmo(1);
        }
    }
    else if (currentAmmo <= 0 && !isReloading)
    {
        Reload();
    }
}
