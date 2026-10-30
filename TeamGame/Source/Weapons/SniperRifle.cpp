#include <cmath>
#include <cstdlib>
#include "SniperRifle.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SoundManager.h"

SniperRifle::SniperRifle() : Weapon("SniperRifle")
{
    barrelVisual.barrelLength = 55.0f;
    barrelVisual.barrelWidth = 3;
    barrelVisual.barrelColor = 0x222B35;
    barrelVisual.accentColor = 0x00E5FF;
    barrelVisual.hasMuzzleBrake = true;
    barrelVisual.muzzleColor = 0x11161B;
    barrelVisual.muzzleWidthOffset = 2.0f;
}

void SniperRifle::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, bool isMoving, const std::string& shooterName, bool isPanic)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene && scene->GetObjectManager())
        {
            // スナイパーの弾丸演出設定（高速シアンビームトレール）
            BulletVisualConfig sniperVisual;
            sniperVisual.customColors = true;
            sniperVisual.coreColor = GetColor(255, 255, 255);
            sniperVisual.glowColor = GetColor(0, 229, 255);       // ネオンシアン
            sniperVisual.glowRadiusOffset = 6.0f;
            sniperVisual.glowAlpha = 220;
            sniperVisual.hasTrail = true;
            sniperVisual.trailColor = GetColor(0, 180, 255);
            sniperVisual.trailAlpha = 200;
            sniperVisual.trailLength = 40.0f;
            sniperVisual.trailWidth = 4;

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

            int dmg = data ? data->damage : 36;
            // 敵を最大2体まで貫通する弾丸を生成
            new Bullet(pos.x, pos.y, finalDir, data->bulletSpeed, data->range, data->bulletRadius,
                       teamId, dmg, shooterName, isPanic, sniperVisual, /*pierce=*/ 2);
            
            // スナイパー特有の重厚な銃声
            SoundManager::GetInstance().Play3D("sniper_shot", pos, 1500.0f, 1.0f, teamId);

            if (scene->GetEffectManager())
            {
                scene->GetEffectManager()->AddMuzzleFlashEffect(pos.x + dir.x * 45.0f, pos.y + dir.y * 45.0f, finalAngle, 28.0f);
            }

            ResetCoolTime();
            UseAmmo(1);
        }
    }
    else if (currentAmmo <= 0 && !isReloading)
    {
        Reload();
    }
}
