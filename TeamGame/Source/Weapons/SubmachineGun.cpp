#include "../Managers/WeaponManager.h"
#include "SubmachineGun.h"
#include "Bullet.h"
#include "ObjectManager.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Enemy.h"
#include "DxLib.h"
#include <cmath>
#include <cstdlib>

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
                enemy->OnHearGunshot(pos, 1000.0f, shooterTeamId);
            }
        }
    }
}

static WeaponData ConfigToWeaponData(const SubmachineGunConfig& config)
{
    WeaponData wd;
    wd.name = config.displayName;
    wd.range = config.range;
    wd.spreadAngle = config.spreadAngle;
    wd.bulletRadius = config.bulletRadius;
    wd.bulletSpeed = config.bulletSpeed;
    wd.maxAmmo = config.maxAmmo;
    wd.fireInterval = config.fireInterval;
    wd.reloadTime = config.reloadTime;
    wd.damage = config.damage;
    wd.pelletCount = config.pelletCount;
    wd.modelPath = config.barrelVisual.modelPath;
    wd.uiImagePath = "NONE";
    wd.uiImageHandle = -1;
    return wd;
}

SubmachineGun::SubmachineGun(const SubmachineGunConfig& config)
    : Weapon(config.displayName, ConfigToWeaponData(config), config.barrelVisual), m_config(config)
{
    ApplyConfig(config);
}

void SubmachineGun::ApplyConfig(const SubmachineGunConfig& config)
{
    m_config = config;
    weaponName = config.displayName;
    customWeaponData = ConfigToWeaponData(config);
    data = &customWeaponData;
    currentAmmo = data->maxAmmo;
    barrelVisual = config.barrelVisual;
}

void SubmachineGun::Fire(const Vector2 &pos, const Vector2 &dir, int teamId, bool isMoving,
                         const std::string& shooterName, bool isPanic)
{
    if (CanFire() && data)
    {
        auto scene = SceneManager::GetInstance().GetCurrentScene();
        if (scene)
        {
            // 移動状態に応じた拡散角度
            float spreadDeg = data->spreadAngle + (isMoving ? m_config.moveSpreadPenalty : 0.0f);
            float spreadRad = spreadDeg * 3.14159265f / 180.0f;
            float angleOffset = ((std::rand() % 1000) / 1000.0f - 0.5f) * spreadRad;
            float baseAngle = std::atan2(dir.y, dir.x);
            float finalAngle = baseAngle + angleOffset;
            Vector2 finalDir(std::cos(finalAngle), std::sin(finalAngle));

            // カスタム描画アセット（弾丸）を適用して生成
            new Bullet(pos.x, pos.y, finalDir, data->bulletSpeed, data->range, data->bulletRadius,
                       teamId, data->damage, shooterName, isPanic, m_config.bulletVisual);

            // マズルフラッシュ演出
            if (scene->GetEffectManager())
            {
                float flashOffset = barrelVisual.barrelLength + 10.0f;
                scene->GetEffectManager()->AddMuzzleFlashEffect(
                    pos.x + dir.x * flashOffset,
                    pos.y + dir.y * flashOffset,
                    finalAngle,
                    m_config.muzzleFlashSize
                );
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

// ====================================================================
// SubmachineGunFactory 実装
// ====================================================================

std::unordered_map<std::string, SubmachineGunConfig>& SubmachineGunFactory::GetRegistry()
{
    static std::unordered_map<std::string, SubmachineGunConfig> registry;
    if (registry.empty())
    {
        registry["CharaA"] = CreateCharaAConfig();
        registry["CharaB"] = CreateCharaBConfig();
        registry["CharaC"] = CreateCharaCConfig();
        registry["CharaD"] = CreateCharaDConfig();
        registry["CharaE"] = CreateCharaEConfig();
    }
    return registry;
}

// ベースSMGパラメータをWeaponManager (weapons.csv) から取得
static SubmachineGunConfig GetBaseSMGConfig()
{
    SubmachineGunConfig cfg;
    const WeaponData* wd = WeaponManager::GetInstance().GetWeaponData("SubmachineGun");
    if (wd)
    {
        cfg.range = wd->range;
        cfg.spreadAngle = wd->spreadAngle;
        cfg.bulletRadius = wd->bulletRadius;
        cfg.bulletSpeed = wd->bulletSpeed;
        cfg.maxAmmo = wd->maxAmmo;
        cfg.fireInterval = wd->fireInterval;
        cfg.reloadTime = wd->reloadTime;
        cfg.damage = wd->damage;
        cfg.pelletCount = wd->pelletCount;
    }
    else
    {
        cfg.range = 200.0f;
        cfg.spreadAngle = 4.5f;
        cfg.bulletRadius = 3.5f;
        cfg.bulletSpeed = 22.0f;
        cfg.maxAmmo = 28;
        cfg.fireInterval = 5;
        cfg.reloadTime = 60;
        cfg.damage = 3;
        cfg.pelletCount = 1;
    }
    return cfg;
}

// キャラA (指揮者 / Tactical Falcon): 高精度・サイバーブルー・プラズマ弾
SubmachineGunConfig SubmachineGunFactory::CreateCharaAConfig()
{
    SubmachineGunConfig cfg = GetBaseSMGConfig();
    cfg.name = "SubmachineGun_CharaA";
    cfg.variantId = "CharaA";
    cfg.displayName = "SMG-Tactical Falcon";
    cfg.description = "高精度・安定射撃。サイバーブルーのプラズマ弾を放つ指揮官カスタム。";
    
    // パラメータ
    cfg.range += 20.0f;              // 射程長め
    cfg.spreadAngle = 2.5f;          // 高精度
    cfg.moveSpreadPenalty = 3.5f;
    cfg.bulletSpeed = 25.0f;
    cfg.damage = 3;

    // 描画アセット (弾)
    cfg.bulletVisual.customColors = true;
    cfg.bulletVisual.coreColor = GetColor(255, 255, 255);
    cfg.bulletVisual.glowColor = GetColor(0, 225, 255);   // ネオンシアン
    cfg.bulletVisual.glowRadiusOffset = 5.0f;
    cfg.bulletVisual.glowAlpha = 180;
    cfg.bulletVisual.hasTrail = true;
    cfg.bulletVisual.trailColor = GetColor(0, 150, 255);
    cfg.bulletVisual.trailAlpha = 140;
    cfg.bulletVisual.trailLength = 18.0f;
    cfg.bulletVisual.trailWidth = 3;

    // 描画アセット (銃身)
    cfg.barrelVisual.barrelLength = 36.0f;
    cfg.barrelVisual.barrelWidth = 4;
    cfg.barrelVisual.barrelColor = GetColor(36, 40, 48);     // チタングレー
    cfg.barrelVisual.accentColor = GetColor(0, 225, 255);    // シアンLED
    cfg.barrelVisual.hasMuzzleBrake = true;
    cfg.barrelVisual.muzzleColor = GetColor(74, 85, 104);
    cfg.barrelVisual.muzzleWidthOffset = 2.0f;
    cfg.muzzleFlashSize = 15.0f;

    return cfg;
}

// キャラB (反逆者 / Crimson Viper): 超速射・クリムゾンレッド・徹甲火炎弾
SubmachineGunConfig SubmachineGunFactory::CreateCharaBConfig()
{
    SubmachineGunConfig cfg;
    cfg.name = "SubmachineGun_CharaB";
    cfg.variantId = "CharaB";
    cfg.displayName = "SMG-Crimson Viper";
    cfg.description = "秒間15発の超速射・多弾数。真紅の徹甲弾で近接を制圧する反逆者カスタム。";

    // パラメータ
    cfg.range = 160.0f;              // 近接寄り
    cfg.spreadAngle = 5.0f;
    cfg.moveSpreadPenalty = 3.5f;    // 移動撃ちに強い
    cfg.bulletRadius = 3.2f;
    cfg.bulletSpeed = 23.0f;
    cfg.maxAmmo = 40;                // 多弾数
    cfg.fireInterval = 4;            // 秒間15発の超高速連射！
    cfg.reloadTime = 40;
    cfg.damage = 3;

    // 描画アセット (弾)
    cfg.bulletVisual.customColors = true;
    cfg.bulletVisual.coreColor = GetColor(255, 200, 200);
    cfg.bulletVisual.glowColor = GetColor(255, 34, 68);    // クリムゾンレッド
    cfg.bulletVisual.glowRadiusOffset = 4.0f;
    cfg.bulletVisual.glowAlpha = 190;
    cfg.bulletVisual.hasTrail = true;
    cfg.bulletVisual.trailColor = GetColor(255, 85, 0);    // フレイムオレンジ
    cfg.bulletVisual.trailAlpha = 150;
    cfg.bulletVisual.trailLength = 14.0f;
    cfg.bulletVisual.trailWidth = 3;

    // 描画アセット (銃身)
    cfg.barrelVisual.barrelLength = 30.0f;                 // 短小コンパクト
    cfg.barrelVisual.barrelWidth = 5;
    cfg.barrelVisual.barrelColor = GetColor(26, 26, 26);   // マットブラック
    cfg.barrelVisual.accentColor = GetColor(255, 23, 68);  // 真紅アクセント
    cfg.barrelVisual.hasMuzzleBrake = true;
    cfg.barrelVisual.muzzleColor = GetColor(50, 16, 16);
    cfg.barrelVisual.muzzleWidthOffset = 3.0f;
    cfg.muzzleFlashSize = 17.0f;

    return cfg;
}

// キャラC (バカ / Heavy Ogre): 大口径重弾・アンバーゴールド・高威力
SubmachineGunConfig SubmachineGunFactory::CreateCharaCConfig()
{
    SubmachineGunConfig cfg;
    cfg.name = "SubmachineGun_CharaC";
    cfg.variantId = "CharaC";
    cfg.displayName = "SMG-Heavy Ogre";
    cfg.description = "大口径のヘビー弾薬を連射。1発2ダメージの高火力を叩き出す重量カスタム。";

    // パラメータ
    cfg.range = 180.0f;
    cfg.spreadAngle = 6.0f;
    cfg.moveSpreadPenalty = 8.0f;
    cfg.bulletRadius = 4.8f;         // 大口径
    cfg.bulletSpeed = 20.0f;
    cfg.maxAmmo = 24;
    cfg.fireInterval = 8;
    cfg.reloadTime = 58;
    cfg.damage = 4;                  // 高威力！

    // 描画アセット (弾)
    cfg.bulletVisual.customColors = true;
    cfg.bulletVisual.coreColor = GetColor(255, 255, 240);
    cfg.bulletVisual.glowColor = GetColor(255, 179, 0);    // アンバーゴールド
    cfg.bulletVisual.glowRadiusOffset = 6.0f;
    cfg.bulletVisual.glowAlpha = 200;
    cfg.bulletVisual.hasTrail = true;
    cfg.bulletVisual.trailColor = GetColor(255, 140, 0);
    cfg.bulletVisual.trailAlpha = 160;
    cfg.bulletVisual.trailLength = 20.0f;
    cfg.bulletVisual.trailWidth = 4;

    // 描画アセット (銃身)
    cfg.barrelVisual.barrelLength = 40.0f;                 // ヘビーバレル
    cfg.barrelVisual.barrelWidth = 6;
    cfg.barrelVisual.barrelColor = GetColor(62, 56, 42);   // オリーブミリタリー
    cfg.barrelVisual.accentColor = GetColor(255, 193, 7);  // ゴールドライン
    cfg.barrelVisual.hasMuzzleBrake = true;
    cfg.barrelVisual.muzzleColor = GetColor(33, 33, 33);
    cfg.barrelVisual.muzzleWidthOffset = 3.5f;
    cfg.muzzleFlashSize = 20.0f;

    return cfg;
}

// キャラD (主婦 / Needle Cleaner): 高弾速・エメラルドグリーン・超速リロード
SubmachineGunConfig SubmachineGunFactory::CreateCharaDConfig()
{
    SubmachineGunConfig cfg;
    cfg.name = "SubmachineGun_CharaD";
    cfg.variantId = "CharaD";
    cfg.displayName = "SMG-Needle Cleaner";
    cfg.description = "極細ニードル弾を超高速連射。0.4秒の超速リロードで息継ぎなしの清掃カスタム。";

    // パラメータ
    cfg.range = 200.0f;
    cfg.spreadAngle = 3.5f;
    cfg.moveSpreadPenalty = 5.0f;
    cfg.bulletRadius = 2.8f;         // ニードル弾
    cfg.bulletSpeed = 30.0f;         // 超高速！
    cfg.maxAmmo = 36;
    cfg.fireInterval = 5;
    cfg.reloadTime = 24;             // 0.4秒の超速リロード！
    cfg.damage = 3;

    // 描画アセット (弾)
    cfg.bulletVisual.customColors = true;
    cfg.bulletVisual.coreColor = GetColor(250, 255, 250);
    cfg.bulletVisual.glowColor = GetColor(0, 230, 118);    // エメラルドグリーン
    cfg.bulletVisual.glowRadiusOffset = 4.0f;
    cfg.bulletVisual.glowAlpha = 180;
    cfg.bulletVisual.hasTrail = true;
    cfg.bulletVisual.trailColor = GetColor(0, 176, 255);
    cfg.bulletVisual.trailAlpha = 130;
    cfg.bulletVisual.trailLength = 22.0f;
    cfg.bulletVisual.trailWidth = 2;

    // 描画アセット (銃身)
    cfg.barrelVisual.barrelLength = 34.0f;
    cfg.barrelVisual.barrelWidth = 3;                      // 細身
    cfg.barrelVisual.barrelColor = GetColor(220, 224, 230); // クロームシルバー
    cfg.barrelVisual.accentColor = GetColor(0, 230, 118);  // ミントグリーン
    cfg.barrelVisual.hasMuzzleBrake = true;
    cfg.barrelVisual.muzzleColor = GetColor(158, 158, 158);
    cfg.barrelVisual.muzzleWidthOffset = 1.5f;
    cfg.muzzleFlashSize = 13.0f;

    return cfg;
}

// キャラE (悪魔 / Inferno Diablos): 呪炎・ダークバイオレット・魔力弾
SubmachineGunConfig SubmachineGunFactory::CreateCharaEConfig()
{
    SubmachineGunConfig cfg;
    cfg.name = "SubmachineGun_CharaE";
    cfg.variantId = "CharaE";
    cfg.displayName = "SMG-Inferno Diablos";
    cfg.description = "禍々しい紫炎の魔力弾を高速連射。長射程と妖気あふれる悪魔の業物カスタム。";

    // パラメータ
    cfg.range = 220.0f;
    cfg.spreadAngle = 3.5f;
    cfg.moveSpreadPenalty = 5.5f;
    cfg.bulletRadius = 3.6f;
    cfg.bulletSpeed = 25.0f;
    cfg.maxAmmo = 33;
    cfg.fireInterval = 5;
    cfg.reloadTime = 48;
    cfg.damage = 3;

    // 描画アセット (弾)
    cfg.bulletVisual.customColors = true;
    cfg.bulletVisual.coreColor = GetColor(255, 224, 255);
    cfg.bulletVisual.glowColor = GetColor(170, 0, 255);    // ダークバイオレット
    cfg.bulletVisual.glowRadiusOffset = 5.5f;
    cfg.bulletVisual.glowAlpha = 190;
    cfg.bulletVisual.hasTrail = true;
    cfg.bulletVisual.trailColor = GetColor(98, 0, 234);
    cfg.bulletVisual.trailAlpha = 150;
    cfg.bulletVisual.trailLength = 19.0f;
    cfg.bulletVisual.trailWidth = 3;

    // 描画アセット (銃身)
    cfg.barrelVisual.barrelLength = 38.0f;
    cfg.barrelVisual.barrelWidth = 5;
    cfg.barrelVisual.barrelColor = GetColor(38, 20, 46);    // 黒紫色
    cfg.barrelVisual.accentColor = GetColor(224, 64, 251); // ネオンマゼンタ
    cfg.barrelVisual.hasMuzzleBrake = true;
    cfg.barrelVisual.muzzleColor = GetColor(74, 20, 140);
    cfg.barrelVisual.muzzleWidthOffset = 2.5f;
    cfg.muzzleFlashSize = 16.0f;

    return cfg;
}

SubmachineGun* SubmachineGunFactory::CreateForCharacter(int characterId)
{
    switch (characterId)
    {
    case 1: return new SubmachineGun(CreateCharaAConfig());
    case 2: return new SubmachineGun(CreateCharaBConfig());
    case 3: return new SubmachineGun(CreateCharaCConfig());
    case 4: return new SubmachineGun(CreateCharaDConfig());
    case 5: return new SubmachineGun(CreateCharaEConfig());
    default: return new SubmachineGun(CreateCharaAConfig());
    }
}

SubmachineGun* SubmachineGunFactory::CreateForCharacter(const std::string& characterName)
{
    if (characterName.find("指揮") != std::string::npos || characterName.find("A") != std::string::npos)
        return new SubmachineGun(CreateCharaAConfig());
    if (characterName.find("反逆") != std::string::npos || characterName.find("B") != std::string::npos)
        return new SubmachineGun(CreateCharaBConfig());
    if (characterName.find("バカ") != std::string::npos || characterName.find("C") != std::string::npos)
        return new SubmachineGun(CreateCharaCConfig());
    if (characterName.find("主婦") != std::string::npos || characterName.find("D") != std::string::npos)
        return new SubmachineGun(CreateCharaDConfig());
    if (characterName.find("悪魔") != std::string::npos || characterName.find("E") != std::string::npos)
        return new SubmachineGun(CreateCharaEConfig());

    return new SubmachineGun(CreateCharaAConfig());
}

SubmachineGun* SubmachineGunFactory::CreateVariant(const std::string& variantId)
{
    auto& reg = GetRegistry();
    auto it = reg.find(variantId);
    if (it != reg.end())
    {
        return new SubmachineGun(it->second);
    }
    return new SubmachineGun(CreateCharaAConfig());
}

void SubmachineGunFactory::RegisterVariant(const std::string& variantId, const SubmachineGunConfig& config)
{
    GetRegistry()[variantId] = config;
}

bool SubmachineGunFactory::HasVariant(const std::string& variantId)
{
    auto& reg = GetRegistry();
    return reg.find(variantId) != reg.end();
}

SubmachineGunConfig SubmachineGunFactory::GetVariantConfig(const std::string& variantId)
{
    auto& reg = GetRegistry();
    auto it = reg.find(variantId);
    if (it != reg.end())
    {
        return it->second;
    }
    return CreateCharaAConfig();
}

std::vector<std::string> SubmachineGunFactory::GetRegisteredVariantIds()
{
    std::vector<std::string> ids;
    for (const auto& pair : GetRegistry())
    {
        ids.push_back(pair.first);
    }
    return ids;
}
