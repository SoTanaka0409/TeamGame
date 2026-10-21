#pragma once
#include "Weapon.h"
#include "BulletVisualConfig.h"
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief サブマシンガン（SMG）の個別派生設定
 * @details キャラクターA〜Eやカスタム派生ごとにパラメータや弾・銃身の描画アセットを一元管理
 */
struct SubmachineGunConfig
{
    std::string name = "SubmachineGun";         ///< 武器名 (例: "SubmachineGun")
    std::string variantId = "Standard";        ///< 派生識別ID (例: "CharaA", "CharaB")
    std::string displayName = "サブマシンガン"; ///< UI表示名 (例: "SMG-Tactical Falcon")
    std::string description = "高速連射が可能な短機関銃"; ///< 特徴・説明

    // --- パラメータ ---
    float range = 180.0f;                      ///< 射程距離
    float spreadAngle = 4.0f;                  ///< 静止時拡散角度（度）
    float moveSpreadPenalty = 6.0f;            ///< 移動時拡散ペナルティ（度）
    float bulletRadius = 3.5f;                 ///< 弾の当たり判定サイズ
    float bulletSpeed = 24.0f;                 ///< 弾速
    int maxAmmo = 32;                          ///< 最大装弾数
    int fireInterval = 6;                      ///< 発射間隔（フレーム数: 60fpsで秒間10発）
    int reloadTime = 48;                       ///< リロード時間（フレーム数: 0.8秒）
    int damage = 1;                            ///< 1発のダメージ
    int pelletCount = 1;                       ///< 1射あたりのペレット数

    // --- 描画アセット (弾丸) ---
    BulletVisualConfig bulletVisual;           ///< 弾のコア色・グロー色・トレール設定

    // --- 描画アセット (銃身) ---
    GunBarrelVisualConfig barrelVisual;        ///< 銃身の長さ・太さ・カラー・LEDライン設定

    // --- 射撃エフェクト演出 ---
    float muzzleFlashSize = 14.0f;             ///< マズルフラッシュサイズ
};

/**
 * @brief サブマシンガン派生クラス
 * @details 各種パラメータおよび弾・銃身の描画アセット設定を内包し、高速フルオート射撃を行います。
 */
class SubmachineGun : public Weapon
{
private:
    SubmachineGunConfig m_config;

public:
    explicit SubmachineGun(const SubmachineGunConfig& config = SubmachineGunConfig());

    void Fire(const Vector2 &pos, const Vector2 &dir, int teamId = 0, bool isMoving = false,
              const std::string& shooterName = "", bool isPanic = false) override;

    float GetMoveSpreadPenalty() const override { return m_config.moveSpreadPenalty; }

    const SubmachineGunConfig& GetConfig() const { return m_config; }
    void ApplyConfig(const SubmachineGunConfig& config);
};

/**
 * @brief サブマシンガン派生ファクトリ＆レジストリ
 * @details キャラクター別プリセットの取得や、新しい派生設定の動的登録・生成を容易に行う仕組み
 */
class SubmachineGunFactory
{
public:
    // --- キャラクター別派生プリセット取得 ---
    static SubmachineGunConfig CreateCharaAConfig(); // キャラA (指揮者/Tactical Falcon): 高精度・サイバーブルー・プラズマ弾
    static SubmachineGunConfig CreateCharaBConfig(); // キャラB (反逆者/Crimson Viper): 超速射・クリムゾンレッド・徹甲火炎弾
    static SubmachineGunConfig CreateCharaCConfig(); // キャラC (バカ/Heavy Ogre): 大口径重弾・アンバーゴールド・高威力
    static SubmachineGunConfig CreateCharaDConfig(); // キャラD (主婦/Needle Cleaner): 高弾速・エメラルドグリーン・超速リロード
    static SubmachineGunConfig CreateCharaEConfig(); // キャラE (悪魔/Inferno Diablos): 呪炎・ダークバイオレット・魔力弾

    // --- インスタンス生成 ---
    static SubmachineGun* CreateForCharacter(int characterId);
    static SubmachineGun* CreateForCharacter(const std::string& characterName);
    static SubmachineGun* CreateVariant(const std::string& variantId);

    // --- 動的登録・カスタマイズAPI ---
    static void RegisterVariant(const std::string& variantId, const SubmachineGunConfig& config);
    static bool HasVariant(const std::string& variantId);
    static SubmachineGunConfig GetVariantConfig(const std::string& variantId);
    static std::vector<std::string> GetRegisteredVariantIds();

private:
    static std::unordered_map<std::string, SubmachineGunConfig>& GetRegistry();
};
