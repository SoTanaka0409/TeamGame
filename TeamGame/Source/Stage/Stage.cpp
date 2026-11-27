#include "Camera.h"
#define NOMINMAX
#include "Stage.h"
#include "DxLib.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace
{
    constexpr int kRockAngleVariants = 2;
    bool s_rocksBaked = false;
    int s_bakedWallRocks[4][kRockAngleVariants] = {
        { -1, -1 },
        { -1, -1 },
        { -1, -1 },
        { -1, -1 }
    };
    int s_bakedSubRocks[4][kRockAngleVariants] = {
        { -1, -1 },
        { -1, -1 },
        { -1, -1 },
        { -1, -1 }
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
        const int texSize = 192;
        const float halfTex = texSize * 0.5f;
        const float bakeCell = texSize * 0.5f;
        const float padOffset = texSize * 0.25f;
        const float rockTilt = 32.0f * (DX_PI_F / 180.0f);

        for (int r = 0; r < 4; ++r)
        {
            int hRock = (hRockModels[r] != -1) ? hRockModels[r] : hRockModels[0];
            float rBase = (hRockModels[r] != -1) ? rockBaseSizes[r] : rockBaseSizes[0];
            if (rBase <= 0.001f) rBase = 7.0f;

            for (int a = 0; a < kRockAngleVariants; ++a)
            {
                float angleSign = (a == 0) ? -1.0f : 1.0f;

                // 1. WALL_BLOCK用3D岩ベイク
                int hWallScr = MakeScreen(texSize, texSize, TRUE);
                if (hWallScr != -1)
                {
                    SetDrawScreen(hWallScr);
                    ClearDrawScreen();
                    SetCameraPositionAndTargetAndUpVec(
                        VGet(halfTex, 2000.0f, halfTex),
                        VGet(halfTex, 0.0f, halfTex),
                        VGet(0.0f, 0.0f, 1.0f)
                    );
                    SetupCamera_Ortho(static_cast<float>(texSize));
                    SetUseLighting(TRUE);
                    SetGlobalAmbientLight(GetColorF(0.82f, 0.85f, 0.82f, 1.0f));
                    ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

                    float s = (bakeCell * 1.28f) / rBase;
                    float cx_f = padOffset + bakeCell * 0.5f;
                    float cy_f = padOffset + bakeCell * 0.66f;
                    float rotY = angleSign * 0.22f;

                    MATRIX mScale = MGetScale(VGet(s, s, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(rockTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(cx_f, 0.0f, static_cast<float>(texSize) - cy_f));
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
                        VGet(halfTex, 2000.0f, halfTex),
                        VGet(halfTex, 0.0f, halfTex),
                        VGet(0.0f, 0.0f, 1.0f)
                    );
                    SetupCamera_Ortho(static_cast<float>(texSize));
                    SetUseLighting(TRUE);
                    SetGlobalAmbientLight(GetColorF(0.82f, 0.85f, 0.82f, 1.0f));
                    ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

                    float s = (bakeCell * 1.12f) / rBase;
                    float cx_f = padOffset + bakeCell * 0.5f;
                    float cy_f = padOffset + bakeCell * 0.64f;
                    float rotY = angleSign * 0.25f;

                    MATRIX mScale = MGetScale(VGet(s, s, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(rockTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(cx_f, 0.0f, static_cast<float>(texSize) - cy_f));
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
        const int texSize = 192;
        const float halfTex = texSize * 0.5f;
        const float bakeCell = texSize * 0.5f;
        const float padOffset = texSize * 0.25f;
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
                    VGet(halfTex, 2000.0f, halfTex),
                    VGet(halfTex, 0.0f, halfTex),
                    VGet(0.0f, 0.0f, 1.0f)
                );
                SetupCamera_Ortho(static_cast<float>(texSize));
                SetUseLighting(TRUE);
                SetGlobalAmbientLight(GetColorF(0.82f, 0.86f, 0.80f, 1.0f));
                ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

                MV1SetOpacityRate(hGrassModel, 1.0f);
                for (int k = 0; k < 5; ++k)
                {
                    float clumpX = padOffset + bakeCell * subOffsetsX[k];
                    float clumpY = padOffset + bakeCell * subOffsetsY[k];
                    float s = unitScale * subScales[k];
                    float rotY = ((v * 73 + k * 53) % 12) * (DX_PI_F / 6.0f);

                    MATRIX mScale = MGetScale(VGet(s, s * 1.25f, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(baseTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(clumpX, 0.0f, static_cast<float>(texSize) - clumpY));
                    mWorld = MMult(mWorld, mTrans);

                    MV1SetMatrix(hGrassModel, mWorld);
                    MV1DrawModel(hGrassModel);
                }

                for (int k = 0; k < 3; ++k)
                {
                    float clumpX = padOffset + bakeCell * fgOffsetsX[k];
                    float clumpY = padOffset + bakeCell * fgOffsetsY[k];
                    float s = unitScale * fgScales[k];
                    float rotY = ((v * 41 + k * 89) % 12) * (DX_PI_F / 6.0f);

                    MATRIX mScale = MGetScale(VGet(s, s * 1.32f, s));
                    MATRIX mRotY  = MGetRotY(rotY);
                    MATRIX mTilt  = MGetRotX(fgTilt);
                    MATRIX mWorld = MMult(mRotY, mTilt);
                    mWorld = MMult(mScale, mWorld);
                    MATRIX mTrans = MGetTranslate(VGet(clumpX, 0.0f, static_cast<float>(texSize) - clumpY));
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

// 【弾丸・射撃を遮断する壁障害物判定】 (水 WATER や 草 BUSH は弾が貫通・すり抜ける)
bool Stage::IsBulletBlockingWall(int gridX, int gridY) const
{
    CellType type = GetCell(gridX, gridY);
    return type == CellType::OUTER_WALL || type == CellType::WALL_BLOCK || type == CellType::CACTUS;
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
                    int aIdx = (x * 53 + y * 29) % kRockAngleVariants;
                    int hTex = s_bakedWallRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
                else if (type == CellType::CACTUS && s_rocksBaked)
                {
                    int rIdx = subRockIndices[(x * 43 + y * 23) % 4];
                    int aIdx = (x * 61 + y * 37) % kRockAngleVariants;
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
    if (m_width <= 0 || m_height <= 0 || worldCellSize <= 0.0f || zoomCellSize <= 0.0f) return;

    int useGrass = (hGrass != -1) ? hGrass : m_hGrassGraph;

    // プレイヤーの位置(playerWorldX, playerWorldY)が画面中央 (960, 540) にくるような基準オフセット
    float startDrawX = Camera::WorldToScreenX(0.0f);
    float startDrawY = Camera::WorldToScreenY(0.0f);

    int sw = 1920, sh = 1080;
    GetDrawScreenSize(&sw, &sh);
    float pad = zoomCellSize * 0.5f;

    // 画面内に映るタイル範囲のみを算出（画面外走査を完全排除）
    int minTileX = (std::max)(0, static_cast<int>(std::floor((-pad - startDrawX) / zoomCellSize)));
    int maxTileX = (std::min)(m_width - 1, static_cast<int>(std::ceil((static_cast<float>(sw) + pad - startDrawX) / zoomCellSize)));
    int minTileY = (std::max)(0, static_cast<int>(std::floor((-pad - startDrawY) / zoomCellSize)));
    int maxTileY = (std::min)(m_height - 1, static_cast<int>(std::ceil((static_cast<float>(sh) + pad - startDrawY) / zoomCellSize)));

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

    // パス1: 不透過ベース（床・外周壁・水場・3D非使用時の2Dオブジェクト）をブレンド切り替えなしで一括描画
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    for (int y = minTileY; y <= maxTileY; ++y)
    {
        float y1_f = startDrawY + y * zoomCellSize;
        int y1 = static_cast<int>(y1_f);
        int y2 = static_cast<int>(y1_f + zoomCellSize);

        for (int x = minTileX; x <= maxTileX; ++x)
        {
            float x1_f = startDrawX + x * zoomCellSize;
            int x1 = static_cast<int>(x1_f);
            int x2 = static_cast<int>(x1_f + zoomCellSize);

            CellType type = GetCell(x, y);

            // 水場・外周壁が完全不透過テクスチャで覆われる場合は床の下描きを省略してフィルレートを削減
            bool coveredByOpaqueTile =
                (type == CellType::WATER && m_hWaterGraph != -1) ||
                (type == CellType::OUTER_WALL && m_hOuterWallGraph != -1);

            if (!coveredByOpaqueTile)
            {
                if (m_hFloorGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hFloorGraph, FALSE);
                }
                else
                {
                    unsigned int fColor = ((x + y) % 2 == 0) ? colorFloor1 : colorFloor2;
                    DrawBox(x1, y1, x2, y2, fColor, TRUE);
                }
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
                    DrawExtendGraph(x1, y1, x2, y2, m_hOuterWallGraph, FALSE);
                }
                else
                {
                    DrawBox(x1, y1, x2, y2, colorOuterWall, TRUE);
                    DrawBox(x1, y1, x2, y2, colorOuterBorder, FALSE);
                    DrawLine(x1, y1, x2, y2, GetColor(90, 100, 120));
                }
                break;

            case CellType::BUSH:
                if (m_hGrassModel == -1)
                {
                    if (useGrass != -1)
                    {
                        DrawExtendGraph(x1 - 1, y1 - 1, x2 + 1, y2 + 1, useGrass, TRUE);
                    }
                    else
                    {
                        DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.65f), colorBush, TRUE);
                        DrawCircle(cx - 3, cy - 3, static_cast<int>(zoomCellSize * 0.40f), colorBushDetail, TRUE);
                    }
                }
                if (isDebugMode) DrawBox(x1, y1, x2, y2, GetColor(100, 240, 100), FALSE);
                break;

            case CellType::WATER:
                if (m_hWaterGraph != -1)
                {
                    DrawExtendGraph(x1, y1, x2, y2, m_hWaterGraph, FALSE);
                }
                else
                {
                    DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.68f), colorWater, TRUE);
                    DrawCircle(cx, cy, static_cast<int>(zoomCellSize * 0.68f), colorWaterBorder, FALSE);
                }
                break;

            case CellType::WALL_BLOCK:
                if (m_hRockModels[0] == -1)
                {
                    if (m_hWallBlockGraph != -1)
                    {
                        DrawExtendGraph(x1, y1, x2, y2, m_hWallBlockGraph, TRUE);
                    }
                    else
                    {
                        DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBlock, TRUE);
                        DrawBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, colorWallBorder, FALSE);
                        DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, GetColor(212, 142, 92), FALSE);
                    }
                }
                break;

            case CellType::CACTUS:
                if (m_hRockModels[0] == -1)
                {
                    if (m_hCactusGraph != -1)
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
                }
                break;
            }
        }
    }

    // パス2: 草むら下地・岩の接地影をSetDrawBlendModeの切り替え最小限でバッチ描画
    if (m_hGrassModel != -1)
    {
        const unsigned int bushBaseCol = GetColor(22, 85, 26);
        const unsigned int bushMidCol  = GetColor(34, 122, 38);
        const int bushMidRad = static_cast<int>(zoomCellSize * 0.56f);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
        for (int y = minTileY; y <= maxTileY; ++y)
        {
            float y1_f = startDrawY + y * zoomCellSize;
            int y1 = static_cast<int>(y1_f);
            int y2 = static_cast<int>(y1_f + zoomCellSize);
            int cy = (y1 + y2) / 2;

            for (int x = minTileX; x <= maxTileX; ++x)
            {
                if (GetCell(x, y) != CellType::BUSH) continue;
                float x1_f = startDrawX + x * zoomCellSize;
                int x1 = static_cast<int>(x1_f);
                int x2 = static_cast<int>(x1_f + zoomCellSize);
                int cx = (x1 + x2) / 2;

                DrawBox(x1, y1, x2, y2, bushBaseCol, TRUE);
                DrawCircle(cx, cy, bushMidRad, bushMidCol, TRUE);
            }
        }
    }

    if (m_hRockModels[0] != -1)
    {
        const unsigned int wallShadowBoxCol = GetColor(42, 36, 30);
        const unsigned int rockShadowCol    = GetColor(30, 26, 22);
        const int wallShadowRad = static_cast<int>(zoomCellSize * 0.48f);
        const int subShadowRad  = static_cast<int>(zoomCellSize * 0.42f);
        const int shadowOffY    = static_cast<int>(zoomCellSize * 0.08f);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
        for (int y = minTileY; y <= maxTileY; ++y)
        {
            float y1_f = startDrawY + y * zoomCellSize;
            int y1 = static_cast<int>(y1_f);
            int y2 = static_cast<int>(y1_f + zoomCellSize);
            int cy = (y1 + y2) / 2 + shadowOffY;

            for (int x = minTileX; x <= maxTileX; ++x)
            {
                CellType type = GetCell(x, y);
                if (type != CellType::WALL_BLOCK && type != CellType::CACTUS) continue;
                float x1_f = startDrawX + x * zoomCellSize;
                int x1 = static_cast<int>(x1_f);
                int x2 = static_cast<int>(x1_f + zoomCellSize);
                int cx = (x1 + x2) / 2;

                if (type == CellType::WALL_BLOCK)
                {
                    DrawBox(x1 + 3, y1 + 3, x2 - 3, y2 - 3, wallShadowBoxCol, TRUE);
                    DrawCircle(cx, cy, wallShadowRad, rockShadowCol, TRUE);
                }
                else
                {
                    DrawCircle(cx, cy, subShadowRad, rockShadowCol, TRUE);
                }
            }
        }
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // パス3: 3D岩モデル (rock_01〜04.mv1) の一括描画（初回3Dベイク済みキャッシュによる高速描画）
    if (m_hRockModels[0] != -1)
    {
        EnsureRockImpostersBaked(m_hRockModels, m_rockModelBaseSizes);

        const int wallRockIndices[4] = { 0, 2, 1, 3 };
        const int subRockIndices[4]  = { 2, 3, 0, 3 };

        for (int y = minTileY; y <= maxTileY; ++y)
        {
            float y1_f = startDrawY + y * zoomCellSize;
            float y2_f = y1_f + zoomCellSize;

            for (int x = minTileX; x <= maxTileX; ++x)
            {
                CellType type = GetCell(x, y);
                if (type != CellType::WALL_BLOCK && type != CellType::CACTUS) continue;

                float x1_f = startDrawX + x * zoomCellSize;
                float x2_f = x1_f + zoomCellSize;

                if (type == CellType::WALL_BLOCK && s_rocksBaked)
                {
                    int rIdx = wallRockIndices[(x * 31 + y * 17) % 4];
                    int aIdx = (x * 53 + y * 29) % kRockAngleVariants;
                    int hTex = s_bakedWallRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
                else if (type == CellType::CACTUS && s_rocksBaked)
                {
                    int rIdx = subRockIndices[(x * 43 + y * 23) % 4];
                    int aIdx = (x * 61 + y * 37) % kRockAngleVariants;
                    int hTex = s_bakedSubRocks[rIdx][aIdx];
                    if (hTex != -1)
                    {
                        DrawExtendGraphF(x1_f - pad, y1_f - pad, x2_f + pad, y2_f + pad, hTex, TRUE);
                    }
                }
            }
        }
    }

    // パス4: 3D草むらモデル (grass.mv1) の描画（遠景はベイクキャッシュ、プレイヤー直近のみリアルタイム3Dかき分け）
    if (m_hGrassModel != -1)
    {
        EnsureGrassImpostersBaked(m_hGrassModel, m_grassModelBaseSize);

        float playerScreenX = Camera::WorldToScreenX(playerWorldX);
        float playerScreenY = Camera::WorldToScreenY(playerWorldY);
        float timeSec   = GetNowCount() / 1000.0f;
        float lodRadius = zoomCellSize * 1.25f;
        float lodRadSq  = lodRadius * lodRadius;

        struct CloseBushCell { int x; int y; float x1_f; float y1_f; };
        CloseBushCell closeBushes[16];
        int closeBushCount = 0;

        // 4A. 遠景の草むらはベイク済み2Dテクスチャで一括描画し、直近セルのみリスト化
        for (int y = minTileY; y <= maxTileY; ++y)
        {
            float y1_f = startDrawY + y * zoomCellSize;
            float y2_f = y1_f + zoomCellSize;
            float cellCenterY = y1_f + pad;
            float cellDy = cellCenterY - playerScreenY;

            for (int x = minTileX; x <= maxTileX; ++x)
            {
                if (GetCell(x, y) != CellType::BUSH) continue;

                float x1_f = startDrawX + x * zoomCellSize;
                float x2_f = x1_f + zoomCellSize;
                float cellCenterX = x1_f + pad;
                float cellDx = cellCenterX - playerScreenX;

                if (cellDx * cellDx + cellDy * cellDy <= lodRadSq && closeBushCount < 16)
                {
                    closeBushes[closeBushCount++] = { x, y, x1_f, y1_f };
                    continue;
                }

                if (s_grassBaked)
                {
                    int vIdx = (x * 3 + y * 7) % 4;
                    int hTex = s_bakedGrassCells[vIdx];
                    if (hTex != -1)
                    {
                        float swayX = std::sin(timeSec * 2.5f + x * 0.6f + y * 0.4f) * (zoomCellSize * 0.025f);
                        DrawExtendGraphF(x1_f - pad + swayX, y1_f - pad, x2_f + pad + swayX, y2_f + pad, hTex, TRUE);
                    }
                }
            }
        }

        // 4B. プレイヤー直近の草むらがある場合のみ3Dカメラを設定してリアルタイムかき分け描画（3株/セル）
        if (closeBushCount > 0)
        {
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

            float unitScale = zoomCellSize / ((m_grassModelBaseSize > 0.001f) ? m_grassModelBaseSize : 1.2f);
            float baseTilt  = 35.0f * (DX_PI_F / 180.0f);
            float interactRadius = zoomCellSize * 0.78f;
            float interactRadSq  = interactRadius * interactRadius;

            const float subOffsetsX[3] = { 0.28f, 0.72f, 0.50f };
            const float subOffsetsY[3] = { 0.32f, 0.34f, 0.56f };
            const float subScales[3]   = { 1.14f, 1.16f, 1.28f };

            for (int i = 0; i < closeBushCount; ++i)
            {
                const auto& b = closeBushes[i];
                for (int k = 0; k < 3; ++k)
                {
                    float clumpX = b.x1_f + zoomCellSize * subOffsetsX[k];
                    float clumpY = b.y1_f + zoomCellSize * subOffsetsY[k];

                    float windWave = std::sin(timeSec * 2.5f + b.x * 0.6f + b.y * 0.4f + k * 0.9f) * 0.10f;

                    float dxScreen = clumpX - playerScreenX;
                    float dyScreen = clumpY - playerScreenY;
                    float distSq   = dxScreen * dxScreen + dyScreen * dyScreen;
                    float rustle   = 0.0f;
                    float pushX    = 0.0f;
                    float pushY    = 0.0f;
                    if (distSq < interactRadSq)
                    {
                        float dist = std::sqrt(distSq);
                        float factor = 1.0f - (dist / interactRadius);
                        rustle = std::sin(timeSec * 15.0f + b.x + b.y + k) * 0.20f * factor;
                        if (dist > 1.0f)
                        {
                            pushX = (dxScreen / dist) * zoomCellSize * 0.22f * factor;
                            pushY = (dyScreen / dist) * zoomCellSize * 0.18f * factor;
                        }
                    }

                    float s = unitScale * subScales[k];
                    float rotY = ((b.x * 73 + b.y * 37 + k * 53) % 12) * (DX_PI_F / 6.0f) + windWave * 0.4f + rustle;
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

    float playerScreenX = Camera::WorldToScreenX(playerWorldX);
    float playerScreenY = Camera::WorldToScreenY(playerWorldY);

    float pad       = zoomCellSize * 0.5f;
    float lodRadius = zoomCellSize * 1.25f;
    float lodRadSq  = lodRadius * lodRadius;

    // プレイヤー直近のグリッド範囲のみを走査
    int playerGridX = static_cast<int>(playerWorldX / worldCellSize);
    int playerGridY = static_cast<int>(playerWorldY / worldCellSize);
    int minGridX = (std::max)(0, playerGridX - 2);
    int maxGridX = (std::min)(m_width - 1, playerGridX + 2);
    int minGridY = (std::max)(0, playerGridY - 2);
    int maxGridY = (std::min)(m_height - 1, playerGridY + 2);

    struct FgBushCell { int x; int y; float x1_f; float y1_f; };
    FgBushCell fgBushes[16];
    int fgCount = 0;

    for (int y = minGridY; y <= maxGridY; ++y)
    {
        for (int x = minGridX; x <= maxGridX; ++x)
        {
            if (GetCell(x, y) != CellType::BUSH) continue;

            float x1_f = startDrawX + x * zoomCellSize;
            float y1_f = startDrawY + y * zoomCellSize;
            float cellDx = (x1_f + pad) - playerScreenX;
            float cellDy = (y1_f + pad) - playerScreenY;
            if (cellDx * cellDx + cellDy * cellDy <= lodRadSq && fgCount < 16)
            {
                fgBushes[fgCount++] = { x, y, x1_f, y1_f };
            }
        }
    }

    // 周辺に草むらがない場合は3Dカメラ設定自体をスキップ
    if (fgCount == 0) return;

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
    float interactRadius = zoomCellSize * 0.72f;
    float interactRadSq  = interactRadius * interactRadius;

    // キャラクターの下半身〜胴体を手前から覆う前景2株
    const float fgOffsetsX[2] = { 0.30f, 0.70f };
    const float fgOffsetsY[2] = { 0.82f, 0.84f };
    const float fgScales[2]   = { 1.22f, 1.24f };

    for (int i = 0; i < fgCount; ++i)
    {
        const auto& b = fgBushes[i];
        for (int k = 0; k < 2; ++k)
        {
            float clumpX = b.x1_f + zoomCellSize * fgOffsetsX[k];
            float clumpY = b.y1_f + zoomCellSize * fgOffsetsY[k];

            float windWave = std::sin(timeSec * 2.6f + b.x * 0.65f + b.y * 0.45f + k * 1.1f) * 0.11f;

            float dxScreen = clumpX - playerScreenX;
            float dyScreen = clumpY - playerScreenY;
            float distSq   = dxScreen * dxScreen + dyScreen * dyScreen;
            float rustle   = 0.0f;
            float pushX    = 0.0f;
            float pushY    = 0.0f;
            float opacity  = 0.94f;
            if (distSq < interactRadSq)
            {
                float dist = std::sqrt(distSq);
                float factor = 1.0f - (dist / interactRadius);
                rustle = std::sin(timeSec * 16.0f + b.x + b.y + k) * 0.22f * factor;
                opacity = 0.94f - 0.18f * factor;
                if (dist > 1.0f)
                {
                    pushX = (dxScreen / dist) * zoomCellSize * 0.18f * factor;
                    pushY = (dyScreen / dist) * zoomCellSize * 0.12f * factor;
                }
            }

            float s = unitScale * fgScales[k];
            float rotY = ((b.x * 41 + b.y * 67 + k * 89) % 12) * (DX_PI_F / 6.0f) + windWave * 0.45f + rustle;
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
    MV1SetOpacityRate(m_hGrassModel, 1.0f);
}
