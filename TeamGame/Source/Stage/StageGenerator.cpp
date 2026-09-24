#include "StageGenerator.h"
#include <algorithm>
#include <ctime>
#include <cmath>

Stage StageGenerator::Generate(const StageGenConfig& config, unsigned int seed, ThemePattern* outTheme, int* outVariation)
{
    if (seed == 0)
    {
        seed = static_cast<unsigned int>(std::time(nullptr));
    }
    std::mt19937 rng(seed);

    // テーマおよびバリエーションを「中央アメーバ大池」単一のカスタムマップに固定
    ThemePattern theme = ThemePattern::CENTER_LAKE;
    int varIdx = 0;

    if (outTheme) *outTheme = theme;
    if (outVariation) *outVariation = varIdx;

    Stage stage;
    stage.Initialize(config.mapWidth, config.mapHeight);

    // 1. 全体を床で初期化
    for (int y = 0; y < config.mapHeight; ++y)
        for (int x = 0; x < config.mapWidth; ++x)
            stage.SetCell(x, y, CellType::EMPTY_FLOOR);

    // 2. 中央アメーバ大池カスタムマップ生成
    GenerateCenterLake(stage, 0, config, rng);

    // 3. 【外枠の封鎖】外周全域に絶対出られない外枠封鎖壁 (OUTER_WALL) を配置
    for (int x = 0; x < config.mapWidth; ++x)
    {
        stage.SetCell(x, 0, CellType::OUTER_WALL);
        stage.SetCell(x, config.mapHeight - 1, CellType::OUTER_WALL);
    }
    for (int y = 0; y < config.mapHeight; ++y)
    {
        stage.SetCell(0, y, CellType::OUTER_WALL);
        stage.SetCell(config.mapWidth - 1, y, CellType::OUTER_WALL);
    }

    // 4. プレイヤー開始位置の設定 (中央★スターの生成は削除)
    int centerY = config.mapHeight / 2;
    stage.SetPlayerStartPos({ 3, centerY });

    // スポーン周囲をクリア
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = 0; dx <= 4; ++dx)
        {
            stage.SetCell(2 + dx, centerY + dy, CellType::EMPTY_FLOOR);
        }
    }

    return stage;
}

std::string StageGenerator::GetThemeName(ThemePattern theme)
{
    return "【中央アメーバ大池】";
}

std::string StageGenerator::GetFullStageName(ThemePattern theme, int variation)
{
    return "【中央アメーバ大池】";
}

void StageGenerator::AddSymmetricOrganicCluster(Stage& stage, CellType type, float cx, float cy, float radiusX, float radiusY, std::mt19937& rng)
{
    int mapW = stage.GetWidth();
    int mapH = stage.GetHeight();

    int startX = std::max(1, static_cast<int>(cx - radiusX - 2));
    int endX   = std::min(mapW - 2, static_cast<int>(cx + radiusX + 2));
    int startY = std::max(1, static_cast<int>(cy - radiusY - 2));
    int endY   = std::min(mapH / 2, static_cast<int>(cy + radiusY + 2));

    for (int y = startY; y <= endY; ++y)
    {
        for (int x = startX; x <= endX; ++x)
        {
            float dx = (x - cx) / radiusX;
            float dy = (y - cy) / radiusY;
            float distSq = dx * dx + dy * dy;

            float noise = (std::sin(x * 0.8f) + std::cos(y * 0.8f)) * 0.15f;
            if (distSq + noise <= 1.0f)
            {
                stage.SetCell(x, y, type);
                int symX = mapW - 1 - x;
                int symY = mapH - 1 - y;
                stage.SetCell(symX, symY, type);
            }
        }
    }
}

// ----------------------------------------------------
// テーマ 1: エメラルド草原 (障害物・壁・茂み増強版)
// ----------------------------------------------------
void StageGenerator::GenerateEmeraldMeadow(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 10.0f, 6.0f, 4.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 9.0f, 10.0f, 3.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 18.0f, 5.0f, 3.5f, 2.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 9.0f, 15.0f, 5.5f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 20.0f, 14.0f, 4.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 12.0f, 4.0f, 1.5f, 1.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 15.0f, 17.0f, 1.4f, 1.4f, rng);
        break;

    case 1:
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 8.0f, 4.0f, 6.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 14.0f, 16.0f, 6.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 12.0f, 8.0f, 4.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 12.0f, 2.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 5.0f, 4.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 10.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 2:
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 6.0f, 5.0f, 5.0f, 5.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 12.0f, 12.0f, 4.5f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 16.0f, 14.0f, 4.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 15.0f, 6.0f, 3.5f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 8.0f, 14.0f, 3.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 18.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 3:
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 10.0f, 8.0f, 5.0f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 7.0f, 15.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 17.0f, 10.0f, 4.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 11.0f, 16.0f, 3.5f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 14.0f, 18.0f, 1.5f, 1.5f, rng);
        break;
    }
}

// ----------------------------------------------------
// テーマ 2: 十字路 & 水場砦 (障害物・壁・水場増強版)
// ----------------------------------------------------
void StageGenerator::GenerateCrossRoads(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 12.0f, 6.0f, 5.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 7.0f, 8.0f, 3.0f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 16.0f, 6.0f, 3.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 10.0f, 15.0f, 5.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 14.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 1:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 8.0f, 7.0f, 4.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 7.0f, 4.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 13.0f, 14.0f, 4.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 15.0f, 3.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 16.0f, 16.0f, 4.5f, 3.5f, rng);
        break;

    case 2:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 14.0f, 5.0f, 6.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 9.0f, 12.0f, 3.5f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 18.0f, 10.0f, 3.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 6.0f, 6.0f, 4.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 18.0f, 15.0f, 1.5f, 1.5f, rng);
        break;

    case 3:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 10.0f, 10.0f, 5.5f, 5.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 10.0f, 10.0f, 2.5f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 17.0f, 13.0f, 3.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 17.0f, 6.0f, 5.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 6.0f, 17.0f, 1.5f, 1.5f, rng);
        break;
    }
}

// ----------------------------------------------------
// テーマ 3: サイド茂み & 丸太砦 (障害物・茂み増強版)
// ----------------------------------------------------
void StageGenerator::GenerateSideForests(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0:
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 7.0f, 5.0f, 6.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 10.0f, 11.0f, 4.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 18.0f, 12.0f, 3.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 16.0f, 4.0f, 4.0f, 3.2f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 8.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 1:
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 9.0f, 3.0f, 5.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 15.0f, 10.0f, 5.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 14.0f, 3.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 12.0f, 16.0f, 3.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 16.0f, 4.0f, 3.0f, rng);
        break;

    case 2:
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 13.0f, 8.0f, 7.5f, 5.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 6.0f, 3.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 17.0f, 15.0f, 3.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 8.0f, 16.0f, 3.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 18.0f, 4.0f, 1.5f, 1.5f, rng);
        break;

    case 3:
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 8.0f, 5.0f, 4.5f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 15.0f, 13.0f, 2.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 11.0f, 10.0f, 3.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 12.0f, 17.0f, 5.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 6.0f, 3.5f, 3.5f, rng);
        break;
    }
}

// ----------------------------------------------------
// テーマ 4: 中央アメーバ大池 (障害物・壁増強版)
// ----------------------------------------------------
void StageGenerator::GenerateCenterLake(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0:
        // 1. 中央アメーバ大池 (水場: WATER)
        AddSymmetricOrganicCluster(stage, CellType::WATER, 24.0f, 13.5f, 7.5f, 4.5f, rng);

        // 2. 橙色のかべ (壁ブロック: WALL_BLOCK)
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 24.0f, 6.0f, 5.0f, 1.8f, rng);  // 上下中央防壁
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 12.0f, 13.5f, 1.8f, 4.5f, rng); // 左右サイド防壁
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 15.0f, 7.5f, 2.5f, 2.0f, rng);  // 斜めカバー防壁

        // 3. 暗灰色の石 (障害物/岩: CACTUS)
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 24.0f, 9.5f, 2.2f, 1.2f, rng);  // 池の上下縁石
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 18.0f, 13.5f, 1.5f, 2.0f, rng); // 池の左右縁石
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 10.0f, 5.0f, 2.0f, 1.8f, rng);  // コーナー岩群

        // 4. 緑色の草むら (身隠し・進入可能茂み: BUSH)
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 6.0f, 4.5f, 4.5f, 3.5f, rng);   // コーナー広域茂み
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 18.0f, 3.5f, 4.0f, 2.5f, rng);  // 上下外周茂み
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 6.0f, 13.5f, 3.5f, 4.5f, rng);  // 左右外周茂み
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 15.0f, 19.5f, 4.0f, 3.0f, rng); // 斜め防衛茂み
        break;

    case 1:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 11.0f, 6.0f, 4.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 16.0f, 14.0f, 5.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 7.0f, 15.0f, 4.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 18.0f, 5.0f, 3.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 8.0f, 10.0f, 3.0f, 2.5f, rng);
        break;

    case 2:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 7.0f, 8.0f, 4.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 15.0f, 7.0f, 4.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 11.0f, 16.0f, 4.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 11.0f, 9.0f, 3.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 16.0f, 15.0f, 5.0f, 4.0f, rng);
        break;

    case 3:
        AddSymmetricOrganicCluster(stage, CellType::WATER, 13.0f, 8.0f, 6.0f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 9.0f, 3.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 17.0f, 14.0f, 3.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 18.0f, 8.0f, 4.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 9.0f, 17.0f, 1.5f, 1.5f, rng);
        break;
    }
}
