#include "Camera.h"
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

// 【移動不可障害物の判定】 (草むら BUSH は進入・歩行可能)
bool Stage::IsSolidWall(int gridX, int gridY) const
{
    CellType type = GetCell(gridX, gridY);
    return type == CellType::OUTER_WALL || type == CellType::WALL_BLOCK || 
           type == CellType::WATER || type == CellType::CACTUS;
}

// 【光を遮断して影を作る壁障害物】 (水 WATER と 草 BUSH は光が奥まで届く)
bool Stage::IsLightBlockingWall(int gridX, int gridY) const
{
    CellType type = GetCell(gridX, gridY);
    return type == CellType::OUTER_WALL || type == CellType::WALL_BLOCK || type == CellType::CACTUS;
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
    const unsigned int colorRock        = GetColor(140, 145, 155);
    const unsigned int colorRockBorder  = GetColor(70, 75, 85);
    const unsigned int colorRockDetail  = GetColor(185, 190, 200);

    // 1. 各タイルのフルカラー標準描画
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
                if (useGrass != -1)
                {
                    DrawExtendGraph(x1 - 1, y1 - 1, x2 + 1, y2 + 1, useGrass, TRUE);
                }
                else
                {
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
                    DrawCircle(cx, cy, r, colorRock, TRUE);
                    DrawCircle(cx, cy, r, colorRockBorder, FALSE);
                    DrawCircle(cx - 2, cy - 2, static_cast<int>(r * 0.45f), colorRockDetail, TRUE);
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
}

void Stage::DrawZoomCamera(float playerWorldX, float playerWorldY, float zoomCellSize, float worldCellSize, bool isDebugMode, const char* patternName, int hGrass) const
{
    if (m_width <= 0 || m_height <= 0 || worldCellSize <= 0.0f) return;

    int useGrass = (hGrass != -1) ? hGrass : m_hGrassGraph;

    // プレイヤーの位置(playerWorldX, playerWorldY)が画面中央 (960, 540) にくるような基準オフセット
    float startDrawX = Camera::WorldToScreenX(0.0f);
    float startDrawY = Camera::WorldToScreenY(0.0f);

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
    const unsigned int colorRock        = GetColor(140, 145, 155);
    const unsigned int colorRockBorder  = GetColor(70, 75, 85);
    const unsigned int colorRockDetail  = GetColor(185, 190, 200);

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            float x1_f = startDrawX + x * zoomCellSize;
            float y1_f = startDrawY + y * zoomCellSize;
            float x2_f = x1_f + zoomCellSize;
            float y2_f = y1_f + zoomCellSize;

            // 画面外のタイルはカリング (高速化)
            if (x2_f < -100.0f || x1_f > 2020.0f || y2_f < -100.0f || y1_f > 1180.0f)
            {
                continue;
            }

            int x1 = static_cast<int>(x1_f);
            int y1 = static_cast<int>(y1_f);
            int x2 = static_cast<int>(x2_f);
            int y2 = static_cast<int>(y2_f);

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
                if (useGrass != -1)
                {
                    DrawExtendGraph(x1 - 1, y1 - 1, x2 + 1, y2 + 1, useGrass, TRUE);
                }
                else
                {
                    DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.65f), colorBush, TRUE);
                    DrawCircle(cx - 3, cy - 3, static_cast<int>(zoomCellSize * 0.40f), colorBushDetail, TRUE);
                }
                if (isDebugMode) DrawBox(x1, y1, x2, y2, GetColor(100, 240, 100), FALSE);
                break;

            case CellType::WATER:
                DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.68f), colorWater, TRUE);
                DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.68f), colorWaterBorder, FALSE);
                break;

            case CellType::WALL_BLOCK:
                DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBlock, TRUE);
                DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBorder, FALSE);
                DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, GetColor(212, 142, 92), FALSE);
                break;

            case CellType::CACTUS:
                {
                    int r = static_cast<int>(zoomCellSize * 0.45f);
                    DrawCircle(cx, cy, r, colorRock, TRUE);
                    DrawCircle(cx, cy, r, colorRockBorder, FALSE);
                    DrawCircle(cx - 2, cy - 2, static_cast<int>(r * 0.45f), colorRockDetail, TRUE);
                }
                break;
            }
        }
    }

    // スポーン要素
    for (const auto& spawn : m_spawnPoints)
    {
        int cx = static_cast<int>(startDrawX + (spawn.gridPos.x + 0.5f) * zoomCellSize);
        int cy = static_cast<int>(startDrawY + (spawn.gridPos.y + 0.5f) * zoomCellSize);

        if (spawn.type == SpawnType::CENTER_STAR)
        {
            int rStar = static_cast<int>(zoomCellSize * 0.9f);
            DrawCircle(cx, cy, rStar, GetColor(0, 115, 255), TRUE);
            DrawCircle(cx, cy, rStar, GetColor(255, 255, 255), FALSE);
            DrawString(cx - 5, cy - 6, "★", GetColor(255, 255, 255));
        }
    }
}
