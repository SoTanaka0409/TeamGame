#define NOMINMAX
#include "LightMaskRenderer.h"
#include "../Core/Camera.h"
#include "../Stage/Stage.h"
#include <DxLib.h>
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kLightMaskW = 240;
    constexpr int kLightMaskH = 135;
    int s_lightMaskSoftImg = -1;
    int s_lightMaskGraph = -1;

    Vector2 NormalizeOrDefault(const Vector2& dir)
    {
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0001f)
        {
            return Vector2(dir.x / len, dir.y / len);
        }
        return Vector2(0.0f, -1.0f);
    }
}

void LightMaskRenderer::ReleaseSharedResources()
{
    if (s_lightMaskGraph != -1)
    {
        DeleteGraph(s_lightMaskGraph);
        s_lightMaskGraph = -1;
    }
    if (s_lightMaskSoftImg != -1)
    {
        DeleteSoftImage(s_lightMaskSoftImg);
        s_lightMaskSoftImg = -1;
    }
}

bool LightMaskRenderer::IsWorldPosInLight(const LightMaskParams& params, const Vector2& targetPos)
{
    if (!params.stage || params.cellSize <= 0.0f) return false;

    const float cellSize = params.cellSize;
    const float dx = targetPos.x - params.worldPos.x;
    const float dy = targetPos.y - params.worldPos.y;
    const float distSq = dx * dx + dy * dy;

    const int stageWidth = params.stage->GetWidth();
    const int stageHeight = params.stage->GetHeight();

    // 遮蔽物レイキャスト判定
    auto isRayBlocked = [&](float toX, float toY, float totalDist) -> bool {
        if (totalDist <= cellSize * 0.5f) return false;
        int raySteps = static_cast<int>(totalDist / (cellSize * 0.35f));
        if (raySteps < 2) raySteps = 2;
        for (int s = 1; s < raySteps; ++s)
        {
            float t = static_cast<float>(s) / static_cast<float>(raySteps);
            float wx = params.worldPos.x + (toX - params.worldPos.x) * t;
            float wy = params.worldPos.y + (toY - params.worldPos.y) * t;
            int gx = static_cast<int>(wx / cellSize);
            int gy = static_cast<int>(wy / cellSize);
            if (gx < 0 || gx >= stageWidth || gy < 0 || gy >= stageHeight ||
                params.stage->IsLightBlockingWall(gx, gy))
            {
                return true;
            }
        }
        return false;
    };

    // 1. 足元の全方位明かり範囲（指揮者仕様時は至近距離のみ）
    const float closeRadius = params.isConductorMode
        ? (params.bodyRadius + 4.0f)
        : (cellSize * params.closeRadiusCells * params.sightMultiplier);
    if (distSq < closeRadius * closeRadius)
    {
        float dist = std::sqrt(distSq);
        if (!isRayBlocked(targetPos.x, targetPos.y, dist))
        {
            return true;
        }
    }

    // 2. 前方扇形スポットライト範囲（指揮者以外かつライトON時のみ）
    if (!params.isConductorMode && params.isLightOn)
    {
        const float maxSpotDist = cellSize * params.maxSpotDistCells * params.sightMultiplier;
        if (distSq < maxSpotDist * maxSpotDist)
        {
            const float dist = std::sqrt(distSq);
            if (dist > 0.001f)
            {
                const Vector2 f = NormalizeOrDefault(params.facingDir);
                const float forwardProj = (dx * f.x + dy * f.y) / dist;
                const float cosHalfAngle = std::cos(params.fanAngleHalf);

                if (forwardProj >= cosHalfAngle)
                {
                    if (!isRayBlocked(targetPos.x, targetPos.y, dist))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void LightMaskRenderer::Render(const LightMaskParams& params, int rectX, int rectY, int rectW, int rectH)
{
    if (!params.stage || params.cellSize <= 0.0f || rectW <= 0 || rectH <= 0) return;

    // 低解像度ARGB8ソフトイメージ＆GPUテクスチャを初回のみ確保（毎フレーム1回のDrawExtendGraphで描画）
    if (s_lightMaskSoftImg == -1)
    {
        s_lightMaskSoftImg = MakeARGB8ColorSoftImage(kLightMaskW, kLightMaskH);
        if (s_lightMaskSoftImg == -1) return;
    }
    if (s_lightMaskGraph == -1)
    {
        FillSoftImage(s_lightMaskSoftImg, 0, 0, 0, 255);
        s_lightMaskGraph = CreateGraphFromSoftImage(s_lightMaskSoftImg);
        if (s_lightMaskGraph == -1) return;
    }

    const float cellSize = params.cellSize;
    const float playerPixelX = Camera::WorldToScreenX(params.worldPos.x);
    const float playerPixelY = Camera::WorldToScreenY(params.worldPos.y);

    const float zoomScale = Camera::ZoomScale;
    const float zoomCellSize = cellSize * zoomScale;

    const bool isConductor = params.isConductorMode;
    const bool spotEnabled = (!isConductor && params.isLightOn);

    const float maxSpotDistWorld = cellSize * params.maxSpotDistCells * params.sightMultiplier;
    const float maxSpotDist = maxSpotDistWorld * zoomScale;
    const float closeRadius = isConductor
        ? (params.bodyRadius * zoomScale + 4.0f)
        : (zoomCellSize * params.closeRadiusCells * params.sightMultiplier);

    const int stageWidth = params.stage->GetWidth();
    const int stageHeight = params.stage->GetHeight();

    // 月明かり（薄明時間帯）の環境光ブーストと最大暗闇アルファ
    const float globalBrightVal = isConductor ? 0.0f : (params.brightRatio * 0.05f);
    const float maxDarkAlpha = isConductor ? 255.0f : (248.0f * (1.0f - params.brightRatio * 0.10f));

    const int darkR = isConductor ? 0 : 4;
    const int darkG = isConductor ? 0 : 5;
    const int darkB = isConductor ? 0 : 10;

    int baseAlpha = 0;
    if (globalBrightVal < 0.98f)
    {
        baseAlpha = static_cast<int>((1.0f - (std::min)(1.0f, globalBrightVal)) * maxDarkAlpha);
        if (baseAlpha <= 8) baseAlpha = 0;
        if (baseAlpha > 255) baseAlpha = 255;
    }

    // まず全面を基本の暗闇アルファで高速塗りつぶし（ライト外の7〜8割の領域はループ自体をスキップ）
    FillSoftImage(s_lightMaskSoftImg, darkR, darkG, darkB, baseAlpha);

    const Vector2 f = NormalizeOrDefault(params.facingDir);
    const float fx = f.x;
    const float fy = f.y;

    // スポットライト扇形の1Dレイキャスト距離テーブルを事前計算
    constexpr int kNumRays = 128;
    float rayMaxDistPx[kNumRays];
    float minScreenX = playerPixelX - closeRadius;
    float maxScreenX = playerPixelX + closeRadius;
    float minScreenY = playerPixelY - closeRadius;
    float maxScreenY = playerPixelY + closeRadius;

    if (spotEnabled && maxSpotDistWorld > 0.001f)
    {
        const float baseAngle = std::atan2(fy, fx);
        float rayStepWorld = cellSize * 0.25f;
        if (rayStepWorld < 4.0f) rayStepWorld = 4.0f;
        const float wallMarginPx = zoomCellSize * 0.25f;

        for (int i = 0; i < kNumRays; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kNumRays - 1);
            const float ang = baseAngle - params.fanAngleHalf + (2.0f * params.fanAngleHalf) * t;
            const float dirX = std::cos(ang);
            const float dirY = std::sin(ang);

            float hitDistWorld = maxSpotDistWorld;
            for (float d = cellSize * 0.5f; d <= maxSpotDistWorld; d += rayStepWorld)
            {
                int gx = static_cast<int>((params.worldPos.x + dirX * d) / cellSize);
                int gy = static_cast<int>((params.worldPos.y + dirY * d) / cellSize);
                if (gx < 0 || gx >= stageWidth || gy < 0 || gy >= stageHeight ||
                    params.stage->IsLightBlockingWall(gx, gy))
                {
                    hitDistWorld = d;
                    break;
                }
            }

            const float reachPx = hitDistWorld * zoomScale;
            rayMaxDistPx[i] = reachPx;

            const float endX = playerPixelX + dirX * (reachPx + wallMarginPx);
            const float endY = playerPixelY + dirY * (reachPx + wallMarginPx);
            if (endX < minScreenX) minScreenX = endX;
            if (endX > maxScreenX) maxScreenX = endX;
            if (endY < minScreenY) minScreenY = endY;
            if (endY > maxScreenY) maxScreenY = endY;
        }
    }

    const float stepX = static_cast<float>(rectW) / static_cast<float>(kLightMaskW);
    const float stepY = static_cast<float>(rectH) / static_cast<float>(kLightMaskH);
    const float invStepX = 1.0f / stepX;
    const float invStepY = 1.0f / stepY;

    int minMx = static_cast<int>((minScreenX - rectX) * invStepX) - 2;
    int maxMx = static_cast<int>((maxScreenX - rectX) * invStepX) + 2;
    int minMy = static_cast<int>((minScreenY - rectY) * invStepY) - 2;
    int maxMy = static_cast<int>((maxScreenY - rectY) * invStepY) + 2;

    if (minMx < 0) minMx = 0;
    if (maxMx >= kLightMaskW) maxMx = kLightMaskW - 1;
    if (minMy < 0) minMy = 0;
    if (maxMy >= kLightMaskH) maxMy = kLightMaskH - 1;

    const float maxSpotDistSq = maxSpotDist * maxSpotDist;
    const float closeRadiusSq = closeRadius * closeRadius;
    const float invCloseRadius = (closeRadius > 0.001f) ? (1.0f / closeRadius) : 0.0f;
    const float invMaxSpotDist = (maxSpotDist > 0.001f) ? (1.0f / maxSpotDist) : 0.0f;
    const float invFanHalf = (params.fanAngleHalf > 0.0001f) ? (1.0f / params.fanAngleHalf) : 0.0f;
    const float rayIndexScale = static_cast<float>(kNumRays - 1) / (2.0f * params.fanAngleHalf);

    const float cosHalfAngle = std::cos(params.fanAngleHalf);
    const float cosHalfAngleSq = cosHalfAngle * cosHalfAngle;
    const float wallEdgePx = zoomCellSize * 0.25f;
    const float invWallEdgePx = (wallEdgePx > 0.001f) ? (1.0f / wallEdgePx) : 0.0f;

    for (int my = minMy; my <= maxMy; ++my)
    {
        const float py = rectY + (my + 0.5f) * stepY;
        const float dy = py - playerPixelY;
        const float dySq = dy * dy;

        for (int mx = minMx; mx <= maxMx; ++mx)
        {
            const float px = rectX + (mx + 0.5f) * stepX;
            const float dx = px - playerPixelX;
            const float distSq = dx * dx + dySq;

            float lightVal = 0.0f;

            // A. プレイヤー周囲の足元明かり
            if (distSq < closeRadiusSq)
            {
                const float dist = std::sqrt(distSq);
                const float ambientLight = 0.38f * (1.0f - (dist * invCloseRadius) * 0.6f);
                lightVal = (std::max)(lightVal, ambientLight);
            }

            // B. 前方扇形スポットライト（1Dレイキャストテーブル参照）
            if (spotEnabled && distSq < maxSpotDistSq)
            {
                const float forwardProj = dx * fx + dy * fy;
                if (forwardProj > 0.0f && (forwardProj * forwardProj >= cosHalfAngleSq * distSq))
                {
                    const float sideProj = -dx * fy + dy * fx;
                    const float relAngle = std::atan2(sideProj, forwardProj);
                    float u = (relAngle + params.fanAngleHalf) * rayIndexScale;
                    if (u < 0.0f) u = 0.0f;
                    if (u > static_cast<float>(kNumRays - 1)) u = static_cast<float>(kNumRays - 1);

                    const int i0 = static_cast<int>(u);
                    const int i1 = (i0 + 1 < kNumRays) ? (i0 + 1) : i0;
                    const float frac = u - static_cast<float>(i0);
                    const float maxReachPx = rayMaxDistPx[i0] + (rayMaxDistPx[i1] - rayMaxDistPx[i0]) * frac;

                    const float dist = std::sqrt(distSq);
                    if (dist <= maxReachPx + wallEdgePx)
                    {
                        const float wallFade = (dist <= maxReachPx)
                            ? 1.0f
                            : (1.0f - (dist - maxReachPx) * invWallEdgePx);
                        float distFade = 1.0f - (dist * invMaxSpotDist);
                        distFade = distFade * distFade;

                        float angleFade = 1.0f - (std::fabs(relAngle) * invFanHalf);
                        if (angleFade < 0.0f) angleFade = 0.0f;

                        const float spotLight = distFade * angleFade * wallFade * 0.95f;
                        lightVal = (std::max)(lightVal, spotLight);
                    }
                }
            }

            if (lightVal > globalBrightVal)
            {
                int alpha = 0;
                if (lightVal < 0.98f)
                {
                    alpha = static_cast<int>((1.0f - (std::min)(1.0f, lightVal)) * maxDarkAlpha);
                    if (alpha <= 8) alpha = 0;
                    if (alpha > 255) alpha = 255;
                }
                if (alpha != baseAlpha)
                {
                    DrawPixelSoftImage_Unsafe_ARGB8(s_lightMaskSoftImg, mx, my, darkR, darkG, darkB, alpha);
                }
            }
        }
    }

    ReCreateGraphFromSoftImage(s_lightMaskSoftImg, s_lightMaskGraph);

    const int prevDrawMode = GetDrawMode();
    SetDrawMode(DX_DRAWMODE_BILINEAR);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawExtendGraph(rectX, rectY, rectX + rectW, rectY + rectH, s_lightMaskGraph, TRUE);
    SetDrawMode(prevDrawMode);
}
