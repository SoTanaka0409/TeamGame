#pragma once
#include <string>

/**
 * @brief 弾丸の描画アセット・視覚演出設定
 * @details キャラクターや武器派生ごとに弾のコア色、グロー色、発光半径、トレール演出を容易に変更可能
 */
struct BulletVisualConfig
{
    bool customColors = false;          ///< trueの場合、チームカラーではなく下記カスタム色を使用
    unsigned int coreColor = 0xFFFFFF;  ///< 弾の中心（コア）色
    unsigned int glowColor = 0x00D5FF;  ///< 弾の外周（グロー）色
    float glowRadiusOffset = 4.0f;      ///< コア半径に対するグローの拡張サイズ (px)
    int glowAlpha = 160;                ///< グローの不透明度 (0~255)
    
    bool hasTrail = true;               ///< 弾道トレール（光の尾・軌跡）を描画するか
    unsigned int trailColor = 0x00A0FF; ///< トレール色
    int trailAlpha = 120;               ///< トレール不透明度 (0~255)
    float trailLength = 16.0f;          ///< トレールの長さ (px)
    int trailWidth = 3;                 ///< トレールの太さ (px)
};

/**
 * @brief 銃身（ガンバレル）の描画アセット設定
 * @details 武器やキャラクター派生ごとに銃身の長さ、太さ、カラーリング、アクセント発光色を変更可能
 */
struct GunBarrelVisualConfig
{
    bool enabled = true;                ///< 銃身描画を有効にするか
    float barrelLength = 38.0f;         ///< 銃身の長さ (px)
    int barrelWidth = 4;                ///< 銃身の太さ (px)
    unsigned int barrelColor = 0x2A2E35;///< 銃身のベースカラー（ガンメタル、スチール、ブラック等）
    unsigned int accentColor = 0x00E5FF;///< アクセント発光ライン／LEDカラー（キャラ固有色）
    bool hasMuzzleBrake = true;         ///< 先端マズルブレーキ（消炎器）を描画するか
    unsigned int muzzleColor = 0x505864;///< マズル先端部の色
    float muzzleWidthOffset = 2.0f;     ///< マズルの拡がり幅
    std::string modelPath = "";         ///< 将来用の3Dモデルパス
};
