#include "Camera.h"
#define NOMINMAX
#include "Stage.h"
#include "DxLib.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace
{
    bool s_rocksBaked = false;
    int s_bakedWallRocks[4][5] = {
        { -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 }
    };
    int s_bakedSubRocks[4][5] = {
        { -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1 }
    };

    bool s_grassBaked = false;
    int s_bakedGrassCells[4] = { -1, -1, -1, -1 };

    /**
     * @brief 高ポリゴンの3D岩モデル(rock_01〜04.mv1)を初回のみ透過オフスクリーンへ3Dレンダリングしてキャッシュする
     */
    void EnsureRockImpostersBaked(const int hRockModels[4], const float rockBaseSizes[4])
    {
        if (s_rocksBaked || hRockModels[0] == -1) return;

        int prevScreen = GetDrawScreen();
        const int texSize = 256;
        const float bakeCell = 128.0f;
        const float rockTilt = 32.0f * (DX_PI_F / 180.0f);

        for (int r = 0; r < 4; ++r)
        {
            int hRock = (hRockModels[r] != -1) ? hRockModels[r] : hRockModels[0];
            float rBase = (hRockModels[r] != -1) ? rockBaseSizes[r] : rockBaseSizes[0];
            if (rBase <= 0.001f) rBase = 7.0f;

            for (int a = 0; a < 5; ++a)
            {
                // 1. WALL_BLOCK用3D岩ベイク
                int hWallScr = MakeScreen(texSize, texSize, TRUE);
                if (hWallScr != -1)
                {
                    SetDrawScreen(hWallScr);
                    ClearDrawScreen();
                    SetCameraPositionAndTargetAndUpVec(
                        VGet(128.0f, 2000.0f, 128.0f),
                        VGet(128.0f, 0.0f, 128.0f),
                        VGet(0.0f, 0.0f, 1.0f)
                    );
                    SetupCamera_Ortho(256.0f);
                    SetUseLighting(TRUE);
                    SetGlobalAmbientLight(GetColorF(0.82f, 0.85f, 0.82f, 1.0f));
                    ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

                    float s = (bakeCell * 1.28f) / rBase;
                    float cx_f = 64.0f + bakeCell * 0.5f;
                    float cy_f = 64.0f + bakeCell * 0.66f;
                    float rotY = (a - 2) * 0.14f;

                    MATRIX mScale = MGetScale(VGet(s, s, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(rockTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(cx_f, 0.0f, 256.0f - cy_f));
                    mWorld = MMult(mWorld, mTrans);

                    MV1SetMatrix(hRock, mWorld);
                    MV1DrawModel(hRock);
                    s_bakedWallRocks[r][a] = hWallScr;
                }

                // 2. CACTUS(内部障害物岩)用3D岩ベイク
                int hSubScr = MakeScreen(texSize, texSize, TRUE);
                if (hSubScr != -1)
                {
                    SetDrawScreen(hSubScr);
                    ClearDrawScreen();
                    SetCameraPositionAndTargetAndUpVec(
                        VGet(128.0f, 2000.0f, 128.0f),
                        VGet(128.0f, 0.0f, 128.0f),
                        VGet(0.0f, 0.0f, 1.0f)
                    );
                    SetupCamera_Ortho(256.0f);
                    SetUseLighting(TRUE);
                    SetGlobalAmbientLight(GetColorF(0.82f, 0.85f, 0.82f, 1.0f));
                    ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

                    float s = (bakeCell * 1.12f) / rBase;
                    float cx_f = 64.0f + bakeCell * 0.5f;
                    float cy_f = 64.0f + bakeCell * 0.64f;
                    float rotY = (a - 2) * 0.16f;

                    MATRIX mScale = MGetScale(VGet(s, s, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(rockTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(cx_f, 0.0f, 256.0f - cy_f));
                    mWorld = MMult(mWorld, mTrans);

                    MV1SetMatrix(hRock, mWorld);
                    MV1DrawModel(hRock);
                    s_bakedSubRocks[r][a] = hSubScr;
                }
            }
        }

        SetDrawScreen(prevScreen);
        s_rocksBaked = true;
    }

    /**
     * @brief 遠景の3D草むらセル(8株構成)を初回のみ透過オフスクリーンへ3Dレンダリングしてキャッシュする
     */
    void EnsureGrassImpostersBaked(int hGrassModel, float grassBaseSize)
    {
        if (s_grassBaked || hGrassModel == -1) return;

        int prevScreen = GetDrawScreen();
        const int texSize = 256;
        const float bakeCell = 128.0f;
        const float unitScale = bakeCell / ((grassBaseSize > 0.001f) ? grassBaseSize : 1.2f);
        const float baseTilt = 35.0f * (DX_PI_F / 180.0f);
        const float fgTilt   = 38.0f * (DX_PI_F / 180.0f);

        const float subOffsetsX[5] = { 0.25f, 0.75f, 0.50f, 0.28f, 0.72f };
        const float subOffsetsY[5] = { 0.28f, 0.28f, 0.45f, 0.62f, 0.62f };
        const float subScales[5]   = { 1.05f, 1.08f, 1.22f, 1.10f, 1.12f };

        const float fgOffsetsX[3]  = { 0.24f, 0.50f, 0.76f };
        const float fgOffsetsY[3]  = { 0.78f, 0.88f, 0.78f };
        const float fgScales[3]    = { 1.15f, 1.26f, 1.15f };

        for (int v = 0; v < 4; ++v)
        {
            int hGrassScr = MakeScreen(texSize, texSize, TRUE);
            if (hGrassScr != -1)
            {
                SetDrawScreen(hGrassScr);
                ClearDrawScreen();
                SetCameraPositionAndTargetAndUpVec(
                    VGet(128.0f, 2000.0f, 128.0f),
                    VGet(128.0f, 0.0f, 128.0f),
                    VGet(0.0f, 0.0f, 1.0f)
                );
                SetupCamera_Ortho(256.0f);
                SetUseLighting(TRUE);
                SetGlobalAmbientLight(GetColorF(0.82f, 0.86f, 0.80f, 1.0f));
                ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

                MV1SetOpacityRate(hGrassModel, 1.0f);
                for (int k = 0; k < 5; ++k)
                {
                    float clumpX = 64.0f + bakeCell * subOffsetsX[k];
                    float clumpY = 64.0f + bakeCell * subOffsetsY[k];
                    float s = unitScale * subScales[k];
                    float rotY = ((v * 73 + k * 53) % 12) * (DX_PI_F / 6.0f);

                    MATRIX mScale = MGetScale(VGet(s, s * 1.25f, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(baseTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(clumpX, 0.0f, 256.0f - clumpY));
                    mWorld = MMult(mWorld, mTrans);

                    MV1SetMatrix(hGrassModel, mWorld);
                    MV1DrawModel(hGrassModel);
                }

                for (int k = 0; k < 3; ++k)
                {
                    float clumpX = 64.0f + bakeCell * fgOffsetsX[k];
                    float clumpY = 64.0f + bakeCell * fgOffsetsY[k];
                    float s = unitScale * fgScales[k];
                    float rotY = ((v * 41 + k * 89) % 12) * (DX_PI_F / 6.0f);

                    MATRIX mScale = MGetScale(VGet(s, s * 1.32f, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(fgTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(clumpX, 0.0f, 256.0f - clumpY));
                    mWorld = MMult(mWorld, mTrans);

                    MV1SetMatrix(hGrassModel, mWorld);
                    MV1DrawModel(hGrassModel);
                }

                s_bakedGrassCells[v] = hGrassScr;
            }
        }

        SetDrawScreen(prevScreen);
        s_grassBaked = true;
    }
}

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

void Stage::ResolveCollision(Vector2& pos, float margin, float cellSize) const
{
    if (cellSize <= 0.0f || m_width <= 0 || m_height <= 0) return;

    for (int iter = 0; iter < 2; ++iter)
    {
        // 1. 左方向
        int leftGridX = static_cast<int>((pos.x - margin) / cellSize);
        int centerGridY = static_cast<int>(pos.y / cellSize);
        if (IsOutOfBounds(leftGridX, centerGridY) || IsSolidWall(leftGridX, centerGridY))
        {
            float wallRightX = (leftGridX + 1) * cellSize;
            pos.x = wallRightX + margin;
        }

        // 2. 右方向
        int rightGridX = static_cast<int>((pos.x + margin) / cellSize);
        if (IsOutOfBounds(rightGridX, centerGridY) || IsSolidWall(rightGridX, centerGridY))
        {
            float wallLeftX = rightGridX * cellSize;
            pos.x = wallLeftX - margin;
        }

        // 3. 上方向
        int centerGridX = static_cast<int>(pos.x / cellSize);
        int topGridY = static_cast<int>((pos.y - margin) / cellSize);
        if (IsOutOfBounds(centerGridX, topGridY) || IsSolidWall(centerGridX, topGridY))
        {
            float wallBottomY = (topGridY + 1) * cellSize;
            pos.y = wallBottomY + margin;
        }

        // 4. 下方向
        int bottomGridY = static_cast<int>((pos.y + margin) / cellSize);
        if (IsOutOfBounds(centerGridX, bottomGridY) || IsSolidWall(centerGridX, bottomGridY))
        {
            float wallTopY = bottomGridY * cellSize;
            pos.y = wallTopY - margin;
        }
    }
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
            if (m_hFloorGraph != -1)
            {
                DrawExtendGraph(x1, y1, x2, y2, m_hFloorGraph, TRUE);
            }
            else
            {
                DrawBox(x1, y1, x2, y2, fColor, TRUE);
            }

            int cx = (x1 + x2) / 2;
            int cy = (y1 + y2) / 2;

            switch (type)
            {
            case CellType::EMPTY_FLOOR:
                if (isDebugMode) DrawBox(x1, y1, x2, y2, GetColor(215, 138, 85), FALSE);
                break;

            case CellType::OUTER_WALL:
                if (m_hOuterWallGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hOuterWallGraph, TRUE);
                }
                else
                {
                    DrawBox(x1, y1, x2, y2, colorOuterWall, TRUE);
                    DrawBox(x1, y1, x2, y2, colorOuterBorder, FALSE);
                    DrawLine(x1, y1, x2, y2, GetColor(90, 100, 120));
                }
                break;

            case CellType::BUSH:
                if (m_hGrassModel != -1)
                {
                    // 3D草むらの下地に深い緑の芝生ベースを敷いて隙間からの茶色床透けを防止
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
                    DrawBox(x1, y1, x2, y2, GetColor(24, 92, 28), TRUE);
                    DrawCircle(cx, cy, static_cast<int>(cellSize * 0.56f), GetColor(34, 128, 38), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else if (useGrass != -1)
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
                if (m_hWaterGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hWaterGraph, TRUE);
                }
                else
                {
                    DrawCircle(cx, cy, static_cast<int>(cellSize * 0.68f), colorWater, TRUE);
                    DrawCircle(cx, cy, static_cast<int>(cellSize * 0.68f), colorWaterBorder, FALSE);
                }
                break;

            case CellType::WALL_BLOCK:
                if (m_hRockModels[0] != -1)
                {
                    // 3D大岩の下地に接地影を描画
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
                    DrawBox(x1 + 2, y1 + 2, x2 - 2, y2 - 2, GetColor(45, 38, 32), TRUE);
                    DrawCircle(cx, cy + static_cast<int>(cellSize * 0.08f), static_cast<int>(cellSize * 0.48f), GetColor(30, 26, 22), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else if (m_hWallBlockGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hWallBlockGraph, TRUE);
                }
                else
                {
                    DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBlock, TRUE);
                    DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBorder, FALSE);
                    DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, GetColor(212, 142, 92), FALSE);
                }
                break;

            case CellType::CACTUS:
                if (m_hRockModels[0] != -1)
                {
                    // 3D岩障害物の下地に丸い接地影を描画
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 135);
                    DrawCircle(cx, cy + static_cast<int>(cellSize * 0.08f), static_cast<int>(cellSize * 0.42f), GetColor(32, 28, 24), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else if (m_hCactusGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hCactusGraph, TRUE);
                }
                else
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

    // 1.5 3D岩モデル＆3D草むらモデルの一括描画（初回3Dベイクキャッシュによる超高速描画）
    if (m_hGrassModel != -1 || m_hRockModels[0] != -1)
    {
        EnsureRockImpostersBaked(m_hRockModels, m_rockModelBaseSizes);
        EnsureGrassImpostersBaked(m_hGrassModel, m_grassModelBaseSize);

        float timeSec = GetNowCount() / 1000.0f;
        float pad = cellSize * 0.5f;
        const int wallRockIndices[4] = { 0, 2, 1, 3 };
        const int subRockIndices[4]  = { 2, 3, 0, 3 };

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                CellType type = GetCell(x, y);
                if (type != CellType::WALL_BLOCK && type != CellType::CACTUS && type != CellType::BUSH) continue;

                float x1_f = startDrawX + x * cellSize;
                float y1_f = startDrawY + y * cellSize;
                float x2_f = x1_f + cellSize;
                float y2_f = y1_f + cellSize;

                if (type == CellType::WALL_BLOCK && s_rocksBaked)
                {
                    int rIdx = wallRockIndices[(x * 31 + y * 17) % 4];
                    int aIdx = (x * 53 + y * 29) % 5;
                    int hTex = s_bakedWallRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
                else if (type == CellType::CACTUS && s_rocksBaked)
                {
                    int rIdx = subRockIndices[(x * 43 + y * 23) % 4];
                    int aIdx = (x * 61 + y * 37) % 5;
                    int hTex = s_bakedSubRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
                else if (type == CellType::BUSH && s_grassBaked)
                {
                    int vIdx = (x * 3 + y * 7) % 4;
                    int hTex = s_bakedGrassCells[vIdx];
                    if (hTex != -1)
                    {
                        float swayX = std::sin(timeSec * 2.4f + x * 0.55f + y * 0.35f) * (cellSize * 0.025f);
                        DrawExtendGraphF(x1_f - pad + swayX, y1_f - pad, x2_f + pad + swayX, y2_f + pad, hTex, TRUE);
                    }
                }
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
            if (m_hStarGraph != -1)
            {
                DrawExtendGraph(cx - rStar, cy - rStar, cx + rStar, cy + rStar, m_hStarGraph, TRUE);
            }
            else
            {
                DrawCircle(cx, cy, rStar, GetColor(0, 115, 255), TRUE);
                DrawCircle(cx, cy, rStar, GetColor(255, 255, 255), FALSE);
                DrawString(cx - 5, cy - 6, "★", GetColor(255, 255, 255));
            }
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
            if (m_hFloorGraph != -1)
            {
                DrawExtendGraph(x1, y1, x2, y2, m_hFloorGraph, TRUE);
            }
            else
            {
                DrawBox(x1, y1, x2, y2, fColor, TRUE);
            }

            int cx = (x1 + x2) / 2;
            int cy = (y1 + y2) / 2;

            switch (type)
            {
            case CellType::EMPTY_FLOOR:
                if (isDebugMode) DrawBox(x1, y1, x2, y2, GetColor(215, 138, 85), FALSE);
                break;

            case CellType::OUTER_WALL:
                if (m_hOuterWallGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hOuterWallGraph, TRUE);
                }
                else
                {
                    DrawBox(x1, y1, x2, y2, colorOuterWall, TRUE);
                    DrawBox(x1, y1, x2, y2, colorOuterBorder, FALSE);
                    DrawLine(x1, y1, x2, y2, GetColor(90, 100, 120));
                }
                break;

            case CellType::BUSH:
                if (m_hGrassModel != -1)
                {
                    // 3D草むらの下地に濃緑の芝生ベースと陰影を描画して立体感を強化
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
                    DrawBox(x1, y1, x2, y2, GetColor(22, 85, 26), TRUE);
                    DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.58f), GetColor(34, 122, 38), TRUE);
                    DrawCircle(cx - 4, cy + 4, static_cast<int>(zoomCellSize * 0.42f), GetColor(18, 68, 22), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else if (useGrass != -1)
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
                if (m_hWaterGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hWaterGraph, TRUE);
                }
                else
                {
                    DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.68f), colorWater, TRUE);
                    DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.68f), colorWaterBorder, FALSE);
                }
                break;

            case CellType::WALL_BLOCK:
                if (m_hRockModels[0] != -1)
                {
                    // 3D大岩の下地に接地影を描画
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 155);
                    DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, GetColor(42, 36, 30), TRUE);
                    DrawCircle(cx, cy + static_cast<int>(zoomCellSize * 0.08f), static_cast<int>(zoomCellSize * 0.48f), GetColor(28, 24, 20), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else if (m_hWallBlockGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hWallBlockGraph, TRUE);
                }
                else
                {
                    DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBlock, TRUE);
                    DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBorder, FALSE);
                    DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, GetColor(212, 142, 92), FALSE);
                }
                break;

            case CellType::CACTUS:
                if (m_hRockModels[0] != -1)
                {
                    // 3D岩障害物の下地に丸い接地影を描画
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
                    DrawCircle(cx, cy + static_cast<int>(zoomCellSize * 0.08f), static_cast<int>(zoomCellSize * 0.42f), GetColor(30, 26, 22), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
                else if (m_hCactusGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hCactusGraph, TRUE);
                }
                else
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

    // 3D岩モデル (rock_01〜04.mv1) の一括描画（初回3Dベイク済みキャッシュによる高速描画）
    if (m_hRockModels[0] != -1)
    {
        EnsureRockImpostersBaked(m_hRockModels, m_rockModelBaseSizes);

        int sw = 1920, sh = 1080;
        GetDrawScreenSize(&sw, &sh);
        float pad = zoomCellSize * 0.5f;
        const int wallRockIndices[4] = { 0, 2, 1, 3 };
        const int subRockIndices[4]  = { 2, 3, 0, 3 };

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                CellType type = GetCell(x, y);
                if (type != CellType::WALL_BLOCK && type != CellType::CACTUS) continue;

                float x1_f = startDrawX + x * zoomCellSize;
                float y1_f = startDrawY + y * zoomCellSize;
                float x2_f = x1_f + zoomCellSize;
                float y2_f = y1_f + zoomCellSize;

                if (x2_f < -pad || x1_f > static_cast<float>(sw) + pad ||
                    y2_f < -pad || y1_f > static_cast<float>(sh) + pad)
                {
                    continue;
                }

                if (type == CellType::WALL_BLOCK && s_rocksBaked)
                {
                    int rIdx = wallRockIndices[(x * 31 + y * 17) % 4];
                    int aIdx = (x * 53 + y * 29) % 5;
                    int hTex = s_bakedWallRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
                else if (type == CellType::CACTUS && s_rocksBaked)
                {
                    int rIdx = subRockIndices[(x * 43 + y * 23) % 4];
                    int aIdx = (x * 61 + y * 37) % 5;
                    int hTex = s_bakedSubRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
            }
        }
    }

    // 3D草むらモデル (grass.mv1) の描画（遠景は3Dベイクキャッシュ、プレイヤー周辺はリアルタイム3Dかき分け描画のハイブリッドLOD）
    if (m_hGrassModel != -1)
    {
        EnsureGrassImpostersBaked(m_hGrassModel, m_grassModelBaseSize);

        int sw = 1920, sh = 1080;
        GetDrawScreenSize(&sw, &sh);
        float centerX = static_cast<float>(sw) / 2.0f;
        float centerY = static_cast<float>(sh) / 2.0f;

        SetCameraPositionAndTargetAndUpVec(
            VGet(centerX, 2000.0f, centerY),
            VGet(centerX, 0.0f, centerY),
            VGet(0.0f, 0.0f, 1.0f)
        );
        SetupCamera_Ortho(static_cast<float>(sh));
        SetUseLighting(TRUE);
        SetGlobalAmbientLight(GetColorF(0.82f, 0.86f, 0.80f, 1.0f));
        ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

        // カメラターゲットではなくプレイヤー自身の正確な画面座標を取得
        float playerScreenX = Camera::WorldToScreenX(playerWorldX);
        float playerScreenY = Camera::WorldToScreenY(playerWorldY);

        float unitScale = zoomCellSize / ((m_grassModelBaseSize > 0.001f) ? m_grassModelBaseSize : 1.2f);
        float timeSec   = GetNowCount() / 1000.0f;
        float baseTilt  = 35.0f * (DX_PI_F / 180.0f);
        float pad       = zoomCellSize * 0.5f;
        float lodRadius = zoomCellSize * 2.25f;
        float lodRadSq  = lodRadius * lodRadius;

        // 1セル内に配置する背面・中層5株の相対オフセットとスケール倍率
        const float subOffsetsX[5] = { 0.25f, 0.75f, 0.50f, 0.28f, 0.72f };
        const float subOffsetsY[5] = { 0.28f, 0.28f, 0.45f, 0.62f, 0.62f };
        const float subScales[5]   = { 1.05f, 1.08f, 1.22f, 1.10f, 1.12f };

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                if (GetCell(x, y) != CellType::BUSH) continue;

                float x1_f = startDrawX + x * zoomCellSize;
                float y1_f = startDrawY + y * zoomCellSize;
                float x2_f = x1_f + zoomCellSize;
                float y2_f = y1_f + zoomCellSize;

                if (x2_f < -pad || x1_f > static_cast<float>(sw) + pad ||
                    y2_f < -pad || y1_f > static_cast<float>(sh) + pad)
                {
                    continue;
                }

                float cellCenterX = x1_f + pad;
                float cellCenterY = y1_f + pad;
                float cellDx = cellCenterX - playerScreenX;
                float cellDy = cellCenterY - playerScreenY;
                float cellDistSq = cellDx * cellDx + cellDy * cellDy;

                // プレイヤーから離れた草むらは8株ベイク済みの3D草テクスチャで超高速描画
                if (cellDistSq > lodRadSq && s_grassBaked)
                {
                    int vIdx = (x * 3 + y * 7) % 4;
                    int hTex = s_bakedGrassCells[vIdx];
                    if (hTex != -1)
                    {
                        float swayX = std::sin(timeSec * 2.5f + x * 0.6f + y * 0.4f) * (zoomCellSize * 0.025f);
                        DrawExtendGraphF(x1_f - pad + swayX, y1_f - pad, x2_f + pad + swayX, y2_f + pad, hTex, TRUE);
                        continue;
                    }
                }

                // プレイヤー周辺の草むらはリアルタイム3Dモデルでかき分け＆揺れを描画
                for (int k = 0; k < 5; ++k)
                {
                    float clumpX = x1_f + zoomCellSize * subOffsetsX[k];
                    float clumpY = y1_f + zoomCellSize * subOffsetsY[k];

                    float windWave = std::sin(timeSec * 2.5f + x * 0.6f + y * 0.4f + k * 0.9f) * 0.10f;

                    // プレイヤーの正確な足元・中心座標との距離で草をかき分ける
                    float dxScreen = clumpX - playerScreenX;
                    float dyScreen = clumpY - playerScreenY;
                    float distSq   = dxScreen * dxScreen + dyScreen * dyScreen;
                    float rustle   = 0.0f;
                    float pushX    = 0.0f;
                    float pushY    = 0.0f;
                    float interactRadius = zoomCellSize * 0.78f;
                    if (distSq < interactRadius * interactRadius)
                    {
                        float dist = std::sqrt(distSq);
                        float factor = 1.0f - (dist / interactRadius);
                        rustle = std::sin(timeSec * 15.0f + x + y + k) * 0.20f * factor;
                        if (dist > 1.0f)
                        {
                            pushX = (dxScreen / dist) * zoomCellSize * 0.22f * factor;
                            pushY = (dyScreen / dist) * zoomCellSize * 0.18f * factor;
                        }
                    }

                    float s = unitScale * subScales[k];
                    float rotY = ((x * 73 + y * 37 + k * 53) % 12) * (DX_PI_F / 6.0f) + windWave * 0.4f + rustle;
                    MATRIX mScale = MGetScale(VGet(s, s * 1.25f, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(baseTilt + windWave + rustle * 0.5f);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(clumpX + pushX, 0.0f, static_cast<float>(sh) - (clumpY + pushY)));
                    mWorld = MMult(mWorld, mTrans);

                    MV1SetMatrix(m_hGrassModel, mWorld);
                    MV1DrawModel(m_hGrassModel);
                }
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
            if (m_hStarGraph != -1)
            {
                DrawExtendGraph(cx - rStar, cy - rStar, cx + rStar, cy + rStar, m_hStarGraph, TRUE);
            }
            else
            {
                DrawCircle(cx, cy, rStar, GetColor(0, 115, 255), TRUE);
                DrawCircle(cx, cy, rStar, GetColor(255, 255, 255), FALSE);
                DrawString(cx - 5, cy - 6, "★", GetColor(255, 255, 255));
            }
        }
    }
}

void Stage::DrawForegroundGrass(float playerWorldX, float playerWorldY, float zoomCellSize, float worldCellSize) const
{
    if (m_width <= 0 || m_height <= 0 || worldCellSize <= 0.0f || m_hGrassModel == -1) return;

    float startDrawX = Camera::WorldToScreenX(0.0f);
    float startDrawY = Camera::WorldToScreenY(0.0f);

    // プレイヤー自身の正確な画面座標
    float playerScreenX = Camera::WorldToScreenX(playerWorldX);
    float playerScreenY = Camera::WorldToScreenY(playerWorldY);

    int sw = 1920, sh = 1080;
    GetDrawScreenSize(&sw, &sh);
    float centerX = static_cast<float>(sw) / 2.0f;
    float centerY = static_cast<float>(sh) / 2.0f;

    SetCameraPositionAndTargetAndUpVec(
        VGet(centerX, 2000.0f, centerY),
        VGet(centerX, 0.0f, centerY),
        VGet(0.0f, 0.0f, 1.0f)
    );
    SetupCamera_Ortho(static_cast<float>(sh));
    SetUseLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.84f, 0.88f, 0.82f, 1.0f));
    ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

    float unitScale = zoomCellSize / ((m_grassModelBaseSize > 0.001f) ? m_grassModelBaseSize : 1.2f);
    float timeSec   = GetNowCount() / 1000.0f;
    float baseTilt  = 38.0f * (DX_PI_F / 180.0f);
    float pad       = zoomCellSize * 0.5f;
    float lodRadius = zoomCellSize * 2.25f;
    float lodRadSq  = lodRadius * lodRadius;

    // プレイヤー周辺のグリッド範囲のみを走査して無駄なループを排除
    int playerGridX = static_cast<int>(playerWorldX / worldCellSize);
    int playerGridY = static_cast<int>(playerWorldY / worldCellSize);
    int minGridX = std::max(0, playerGridX - 3);
    int maxGridX = std::min(m_width - 1, playerGridX + 3);
    int minGridY = std::max(0, playerGridY - 3);
    int maxGridY = std::min(m_height - 1, playerGridY + 3);

    // キャラクターの下半身〜胴体を手前から覆い隠して「草むらに潜っている感」を出す前景3株
    const float fgOffsetsX[3] = { 0.24f, 0.50f, 0.76f };
    const float fgOffsetsY[3] = { 0.78f, 0.88f, 0.78f };
    const float fgScales[3]   = { 1.15f, 1.26f, 1.15f };

    for (int y = minGridY; y <= maxGridY; ++y)
    {
        for (int x = minGridX; x <= maxGridX; ++x)
        {
            if (GetCell(x, y) != CellType::BUSH) continue;

            float x1_f = startDrawX + x * zoomCellSize;
            float y1_f = startDrawY + y * zoomCellSize;

            float cellCenterX = x1_f + pad;
            float cellCenterY = y1_f + pad;
            float cellDx = cellCenterX - playerScreenX;
            float cellDy = cellCenterY - playerScreenY;
            if (cellDx * cellDx + cellDy * cellDy > lodRadSq)
            {
                continue;
            }

            for (int k = 0; k < 3; ++k)
            {
                float clumpX = x1_f + zoomCellSize * fgOffsetsX[k];
                float clumpY = y1_f + zoomCellSize * fgOffsetsY[k];

                float windWave = std::sin(timeSec * 2.6f + x * 0.65f + y * 0.45f + k * 1.1f) * 0.11f;

                float dxScreen = clumpX - playerScreenX;
                float dyScreen = clumpY - playerScreenY;
                float distSq   = dxScreen * dxScreen + dyScreen * dyScreen;
                float rustle   = 0.0f;
                float pushX    = 0.0f;
                float pushY    = 0.0f;
                float opacity  = 0.94f;
                float interactRadius = zoomCellSize * 0.72f;
                if (distSq < interactRadius * interactRadius)
                {
                    float dist = std::sqrt(distSq);
                    float factor = 1.0f - (dist / interactRadius);
                    rustle = std::sin(timeSec * 16.0f + x + y + k) * 0.22f * factor;
                    // プレイヤーの頭・上半身が草むらの中から覗くように、直近の草だけわずかに左右へ分けて半透過にする
                    opacity = 0.94f - 0.18f * factor;
                    if (dist > 1.0f)
                    {
                        pushX = (dxScreen / dist) * zoomCellSize * 0.18f * factor;
                        pushY = (dyScreen / dist) * zoomCellSize * 0.12f * factor;
                    }
                }

                float s = unitScale * fgScales[k];
                float rotY = ((x * 41 + y * 67 + k * 89) % 12) * (DX_PI_F / 6.0f) + windWave * 0.45f + rustle;
                MATRIX mScale = MGetScale(VGet(s, s * 1.32f, s));
                MATRIX mRotY  = MGetRotY(rotY);
                MATRIX mTilt  = MGetRotX(baseTilt + windWave + rustle * 0.55f);
                MATRIX mWorld = MMult(mRotY, mTilt);
                mWorld = MMult(mScale, mWorld);
                MATRIX mTrans = MGetTranslate(VGet(clumpX + pushX, 0.0f, static_cast<float>(sh) - (clumpY + pushY)));
                mWorld = MMult(mWorld, mTrans);

                MV1SetOpacityRate(m_hGrassModel, opacity);
                MV1SetMatrix(m_hGrassModel, mWorld);
                MV1DrawModel(m_hGrassModel);
            }
        }
    }
    MV1SetOpacityRate(m_hGrassModel, 1.0f);
}
