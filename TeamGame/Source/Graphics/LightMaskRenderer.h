#pragma once
#include "../Core/Vector2.h"

class Stage;

/**
 * @brief ライト・視界マスク描画および視界判定に必要なパラメータ一式
 * @details Playerクラスに直接依存せず、任意の光源・キャラクターへ拡張可能な値型構造体です。
 */
struct LightMaskParams
{
    const Stage* stage = nullptr;   ///< 遮光判定対象のステージ
    Vector2 worldPos{ 0.0f, 0.0f }; ///< 光源（キャラクター）のワールド座標
    Vector2 facingDir{ 0.0f, -1.0f }; ///< スポットライト照射方向（単位ベクトル推奨）
    float cellSize = 40.0f;         ///< ステージの1セルサイズ（ワールド単位）
    float bodyRadius = 8.9f;        ///< キャラクター本体半径
    float closeRadiusCells = 1.6f;  ///< 足元の全方位明かり半径（セル数）
    float maxSpotDistCells = 8.5f;  ///< 前方スポットライト最大照射距離（セル数）
    float fanAngleHalf = 0.5236f;   ///< スポットライト半角（ラジアン、約30°）
    float sightMultiplier = 1.0f;   ///< スキル等による視界倍率
    float brightRatio = 0.0f;       ///< 薄明イベント等の環境光比率 (0.0〜1.0)
    bool isLightOn = true;          ///< スポットライト点灯フラグ
    bool isConductorMode = false;   ///< 指揮者仕様（漆黒・ライト照射なし）
};

/**
 * @brief 動的暗闇マスク＆懐中電灯スポットライトの専用レンダラー
 * @details 1Dレイキャスト距離テーブルと低解像度ARGB8ソフトイメージ（1DrawCall転送）により、
 *          高フレームレートと滑らかな影境界を両立します。
 */
class LightMaskRenderer
{
public:
    /**
     * @brief 指定したワールド座標が光源の可視範囲（足元明かり or スポットライト）内にあるか判定
     */
    static bool IsWorldPosInLight(const LightMaskParams& params, const Vector2& targetPos);

    /**
     * @brief 画面領域に対して暗闇＆スポットライトマスクを合成描画
     */
    static void Render(const LightMaskParams& params, int rectX, int rectY, int rectW, int rectH);

    /**
     * @brief 共有オフスクリーンリソース（SoftImage / Graph）を明示解放
     */
    static void ReleaseSharedResources();
};
