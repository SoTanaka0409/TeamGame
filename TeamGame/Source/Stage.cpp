#define NOMINMAX
#include "Stage.h"
#include "DxLib.h"
#include <cmath>
#include <vector>
#include <algorithm>

Stage::Stage()
    : m_width(0), m_height(0)
{
}

void Stage::Initialize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_grid.assign(width * height, CellType::EMPTY_FLOOR);
    m_spawnPoints.clear();
}

CellType Stage::GetCell(int x, int y) const
{
    if (IsOutOfBounds(x, y)) return CellType::OUTER_WALL;
    return m_grid[y * m_width + x];
}

void Stage::SetCell(int x, int y, CellType type)
{
    if (IsOutOfBounds(x, y)) return;
    m_grid[y * m_width + x] = type;
}

void Stage::AddSpawnPoint(Point2D pos, SpawnType type)
{
    m_spawnPoints.push_back({ pos, type });
}

bool Stage::IsSolidWall(int gridX, int gridY) const
{
    CellType type = GetCell(gridX, gridY);
    return type == CellType::OUTER_WALL || type == CellType::WALL_BLOCK || 
           type == CellType::WATER || type == CellType::CACTUS || 
           type == CellType::BUSH;
}

bool Stage::IsOutOfBounds(int gridX, int gridY) const
{
    return (gridX < 0 || gridX >= m_width || gridY < 0 || gridY >= m_height);
}

void Stage::DrawFitToArea(int rectX, int rectY, int rectW, int rectH, bool isDebugMode, float playerWorldX, float playerWorldY, float lightAngle, const char* patternName, int hGrass) const
{
    if (m_width <= 0 || m_height <= 0) return;

    int useGrass = (hGrass != -1) ? hGrass : m_hGrassGraph;

    // 1920x1080 フィッティング計算
    float cellW = static_cast<float>(rectW) / m_width;
    float cellH = static_cast<float>(rectH) / m_height;
    float cellSize = (cellW < cellH) ? cellW : cellH;

    float mapPixelWidth = m_width * cellSize;
    float mapPixelHeight = m_height * cellSize;
    float startDrawX = rectX + (rectW - mapPixelWidth) / 2.0f;
    float startDrawY = rectY + (rectH - mapPixelHeight) / 2.0f;

    // プレイヤーのピクセル座標
    float playerPixelX = startDrawX + playerWorldX * cellSize;
    float playerPixelY = startDrawY + playerWorldY * cellSize;

    // カラーパレット
    const unsigned int colorFloor1      = GetColor(242, 162, 108);
    const unsigned int colorFloor2      = GetColor(232, 150, 95);
    const unsigned int colorBush        = GetColor(45, 178, 48);
    const unsigned int colorBushDetail  = GetColor(25, 142, 28);
    const unsigned int colorWater       = GetColor(40, 152, 242);
    const unsigned int colorWaterBorder = GetColor(18, 112, 204);
    const unsigned int colorWallBlock   = GetColor(192, 122, 78);
    const unsigned int colorWallBorder  = GetColor(132, 72, 38);
    const unsigned int colorOuterWall   = GetColor(65, 70, 85);
    const unsigned int colorOuterBorder = GetColor(130, 140, 160);
    const unsigned int colorCactus      = GetColor(40, 160, 80);

    // 1. 各タイルのフルカラー標準描画（BUSHタイルは Grass1.png 画像を使用）
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            int x1 = static_cast<int>(startDrawX + x * cellSize);
            int y1 = static_cast<int>(startDrawY + y * cellSize);
            int x2 = static_cast<int>(startDrawX + (x + 1) * cellSize);
            int y2 = static_cast<int>(startDrawY + (y + 1) * cellSize);

            CellType type = GetCell(x, y);

            unsigned int fColor = ((x + y) % 2 == 0) ? colorFloor1 : colorFloor2;
            DrawBox(x1, y1, x2, y2, fColor, TRUE);

            int cx = (x1 + x2) / 2;
            int cy = (y1 + y2) / 2;

            switch (type)
            {
            case CellType::EMPTY_FLOOR:
                if (isDebugMode) DrawBox(x1, y1, x2, y2, GetColor(215, 138, 85), FALSE);
                break;

            case CellType::OUTER_WALL:
                DrawBox(x1, y1, x2, y2, colorOuterWall, TRUE);
                DrawBox(x1, y1, x2, y2, colorOuterBorder, FALSE);
                DrawLine(x1, y1, x2, y2, GetColor(90, 100, 120));
                break;

            case CellType::BUSH:
                // 【Grass1.png 画像テクスチャ描画】
                if (useGrass != -1)
                {
                    // 画像で草むらを描画
                    DrawExtendGraph(x1 - 1, y1 - 1, x2 + 1, y2 + 1, useGrass, TRUE);
                }
                else
                {
                    // フォールバックベクター描画
                    DrawCircle(cx, cy, static_cast<int>(cellSize * 0.65f), colorBush, TRUE);
                    DrawCircle(cx - 3, cy - 3, static_cast<int>(cellSize * 0.40f), colorBushDetail, TRUE);
                }
                if (isDebugMode) DrawBox(x1, y1, x2, y2, GetColor(100, 240, 100), FALSE);
                break;

            case CellType::WATER:
                DrawCircle(cx, cy, static_cast<int>(cellSize * 0.68f), colorWater, TRUE);
                DrawCircle(cx, cy, static_cast<int>(cellSize * 0.68f), colorWaterBorder, FALSE);
                break;

            case CellType::WALL_BLOCK:
                DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBlock, TRUE);
                DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBorder, FALSE);
                DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, GetColor(212, 142, 92), FALSE);
                break;

            case CellType::CACTUS:
                {
                    int r = static_cast<int>(cellSize * 0.45f);
                    DrawCircle(cx, cy, r, colorCactus, TRUE);
                    DrawCircle(cx, cy, r, GetColor(20, 100, 40), FALSE);
                }
                break;
            }
        }
    }

    // 2. スポーン要素の描画
    for (const auto& spawn : m_spawnPoints)
    {
        int cx = static_cast<int>(startDrawX + (spawn.gridPos.x + 0.5f) * cellSize);
        int cy = static_cast<int>(startDrawY + (spawn.gridPos.y + 0.5f) * cellSize);

        if (spawn.type == SpawnType::CENTER_STAR)
        {
            int rStar = static_cast<int>(cellSize * 0.9f);
            DrawCircle(cx, cy, rStar, GetColor(0, 115, 255), TRUE);
            DrawCircle(cx, cy, rStar, GetColor(255, 255, 255), FALSE);
            DrawString(cx - 5, cy - 6, "★", GetColor(255, 255, 255));
        }
    }

    // 3. 【高精度ピクセル描画暗闇マスク】 (通常時)
    if (!isDebugMode)
    {
        float maxSpotDist = cellSize * 14.0f; // 60度扇形ライト射程
        float closeRadius = cellSize * 2.0f;   // 足元のやや見える円半径
        float fanAngleHalf = 0.5236f;          // 60度 (±30度)

        int resolutionStep = 2; // 高解像度 2px

        for (int py = rectY; py < rectY + rectH; py += resolutionStep)
        {
            for (int px = rectX; px < rectX + rectW; px += resolutionStep)
            {
                float dx = px - playerPixelX;
                float dy = py - playerPixelY;
                float dist = std::sqrt(dx * dx + dy * dy);

                float lightVal = 0.0f;

                // A. プレイヤー周囲の控えめな円形明かり
                float ambientLight = 0.0f;
                if (dist < closeRadius)
                {
                    ambientLight = 0.38f * (1.0f - (dist / closeRadius) * 0.6f);
                }

                // B. 前方60°扇形スポットライト
                float cellAngle = std::atan2(dy, dx);
                float angleDiff = std::abs(cellAngle - lightAngle);
                while (angleDiff > 3.14159265f) angleDiff = std::abs(angleDiff - 2.0f * 3.14159265f);

                float spotLight = 0.0f;
                if (dist < maxSpotDist && angleDiff < fanAngleHalf)
                {
                    float distFade = 1.0f - (dist / maxSpotDist);
                    distFade = distFade * distFade;
                    float angleFade = 1.0f - (angleDiff / fanAngleHalf);
                    spotLight = distFade * angleFade * 0.95f;
                }

                lightVal = std::max(ambientLight, spotLight);

                if (lightVal < 0.98f)
                {
                    int alpha = static_cast<int>((1.0f - std::min(1.0f, lightVal)) * 248);
                    if (alpha > 8)
                    {
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                        DrawBox(px, py, px + resolutionStep, py + resolutionStep, GetColor(4, 5, 10), TRUE);
                    }
                }
            }
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 4. 【デバッグプレイヤー P】 の描画
    int px = static_cast<int>(playerPixelX);
    int py = static_cast<int>(playerPixelY);
    
    int pr = static_cast<int>(cellSize * 0.55f);
    if (pr < 4) pr = 4;

    DrawCircle(px, py, pr + 1, GetColor(255, 255, 255), FALSE);
    DrawCircle(px, py, pr, GetColor(0, 220, 100), TRUE);
    DrawCircle(px, py, pr, GetColor(0, 120, 50), FALSE);
    DrawString(px - 4, py - 6, "P", GetColor(0, 0, 0));
}
