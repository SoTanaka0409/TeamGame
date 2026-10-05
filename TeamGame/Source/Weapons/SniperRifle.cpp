#include <cmath>
#include <cstdlib>
#include "SniperRifle.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SoundManager.h"

/**
 * @brief SniperRifleのコンストラクタ
 * @details 親クラスWeaponを"SniperRifle"という名前で初期化する
 */
SniperRifle::SniperRifle() : Weapon("SniperRifle") {}

/**
 * @brief スナイパーの弾を発射する実装
 * @param pos 発射位置
 * @param dir 発射方向
 * @param teamId チームID
 * @param additionalSpread 追加の拡散角度
 * @details ブレを考慮しつつ高速・長射程のBulletオブジェクトを生成し、スナイパー特有の銃声を鳴らす
 */
void SniperRifle::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, bool isMoving)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            float additionalSpread = isMoving ? GetMoveSpreadPenalty() : 0.0f;
            float totalSpread = data->spreadAngle + additionalSpread;
            float halfSpreadRad = (totalSpread / 2.0f) * (3.14159265f / 180.0f);
            float randomAngle = 0.0f;
            if (halfSpreadRad > 0.0f) {
                randomAngle = (((float)std::rand() / RAND_MAX) * (halfSpreadRad * 2.0f)) - halfSpreadRad;
            }
            float currentAngle = std::atan2(dir.y, dir.x);
            float finalAngle = currentAngle + randomAngle;
            Vector2 finalDir(std::cos(finalAngle), std::sin(finalAngle));

            int dmg = data ? data->damage : 3;
            Bullet* bullet = new Bullet(pos.x, pos.y, finalDir, data->bulletSpeed, data->range, data->bulletRadius, teamId, dmg);
            
            // スナイパー特有の銃声
            SoundManager::GetInstance().Play3D("sniper_shot", pos, 1500.0f, 1.0f, teamId);

            ResetCoolTime();
            UseAmmo(1);
        }
    }
    else if (currentAmmo <= 0 && !isReloading)
    {
        Reload();
    }
}
