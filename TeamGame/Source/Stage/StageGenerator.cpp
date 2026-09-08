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

    ThemePattern theme = config.theme;
    int varIdx = config.variation % 4;

    if (outTheme) *outTheme = theme;
    if (outVariation) *outVariation = varIdx;

    Stage stage;
    stage.Initialize(config.mapWidth, config.mapHeight);

    // 1. 全体を床で初期化
    for (int y = 0; y < config.mapHeight; ++y)
        for (int x = 0; x < config.mapWidth; ++x)
            stage.SetCell(x, y, CellType::EMPTY_FLOOR);

    // 2. 4テーマ × 4バリエーションレイアウト生成
    switch (theme)
    {
    case ThemePattern::EMERALD_MEADOW:
        GenerateEmeraldMeadow(stage, varIdx, config, rng);
        break;
    case ThemePattern::CROSS_ROADS:
        GenerateCrossRoads(stage, varIdx, config, rng);
        break;
    case ThemePattern::SIDE_FORESTS:
        GenerateSideForests(stage, varIdx, config, rng);
        break;
    case ThemePattern::CENTER_LAKE:
        GenerateCenterLake(stage, varIdx, config, rng);
        break;
    }

    // 3. 【外枠の封鎖】外周全域に外枠封鎖壁 (OUTER_WALL) を配置
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

    // 4. プレイヤー開始位置と中央スターの設定
    int centerY = config.mapHeight / 2;
    stage.SetPlayerStartPos({ 3, centerY });
    stage.AddSpawnPoint({ config.mapWidth / 2, centerY }, SpawnType::CENTER_STAR);

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
    switch (theme)
    {
    case ThemePattern::EMERALD_MEADOW: return "【テーマ 1: エメラルド草原】";
    case ThemePattern::CROSS_ROADS:     return "【テーマ 2: 十字路 & 水場砦】";
    case ThemePattern::SIDE_FORESTS:    return "【テーマ 3: サイド茂み & 丸太砦】";
    case ThemePattern::CENTER_LAKE:     return "【テーマ 4: 中央アメーバ大池】";
    }
    return "【標準テーマ】";
}

std::string StageGenerator::GetFullStageName(ThemePattern theme, int variation)
{
    std::string base = GetThemeName(theme);
    std::string varStr = " (ステージ " + std::to_string(variation + 1) + " / 4)";
    return base + varStr;
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
// テーマ 1: エメラルド草原 (4つのバリエーションステージ)
// ----------------------------------------------------
void StageGenerator::GenerateEmeraldMeadow(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0: // 1-A: オーソドックス双子池 & 中央エメラルド茂み
        AddSymmetricOrganicCluster(stage, CellType::WATER, 10.0f, 6.0f, 4.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 9.0f, 10.0f, 2.5f, 2.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 9.0f, 15.0f, 5.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 12.0f, 4.0f, 1.2f, 1.2f, rng);
        break;

    case 1: // 1-B: 外周茂みレーン & 散在木箱
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 8.0f, 4.0f, 6.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 14.0f, 16.0f, 5.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 12.0f, 8.0f, 3.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 5.0f, 3.0f, 3.0f, rng);
        break;

    case 2: // 1-C: 中央サンド広場 & コーナー森林
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 6.0f, 5.0f, 4.5f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 16.0f, 14.0f, 4.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 15.0f, 6.0f, 2.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 8.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 3: // 1-D: 草原迷路 & アメーバ池
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 10.0f, 8.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 7.0f, 15.0f, 3.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 17.0f, 10.0f, 3.5f, 2.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 14.0f, 18.0f, 1.3f, 1.3f, rng);
        break;
    }
}

// ----------------------------------------------------
// テーマ 2: 十字路 & 水場砦 (4つのバリエーションステージ)
// ----------------------------------------------------
void StageGenerator::GenerateCrossRoads(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0: // 2-A: 十字路中央ウォーター砦
        AddSymmetricOrganicCluster(stage, CellType::WATER, 12.0f, 6.0f, 5.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 7.0f, 8.0f, 2.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 10.0f, 15.0f, 4.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 14.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 1: // 2-B: 左右ダブルリング水場
        AddSymmetricOrganicCluster(stage, CellType::WATER, 8.0f, 7.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 7.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 13.0f, 14.0f, 3.5f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 16.0f, 16.0f, 4.0f, 3.0f, rng);
        break;

    case 2: // 2-C: S字ウォーターライン
        AddSymmetricOrganicCluster(stage, CellType::WATER, 14.0f, 5.0f, 6.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 9.0f, 12.0f, 3.0f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 6.0f, 6.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 18.0f, 15.0f, 1.4f, 1.4f, rng);
        break;

    case 3: // 2-D: アイランド砦水場
        AddSymmetricOrganicCluster(stage, CellType::WATER, 10.0f, 10.0f, 5.0f, 5.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 10.0f, 10.0f, 2.0f, 2.0f, rng); // 中央アイランド
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 17.0f, 6.0f, 4.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 6.0f, 17.0f, 1.2f, 1.2f, rng);
        break;
    }
}

// ----------------------------------------------------
// テーマ 3: サイド茂み & 丸太砦 (4つのバリエーションステージ)
// ----------------------------------------------------
void StageGenerator::GenerateSideForests(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0: // 3-A: 左右密林レーン
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 7.0f, 5.0f, 6.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 10.0f, 11.0f, 3.5f, 2.2f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 16.0f, 4.0f, 3.5f, 2.8f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 8.0f, 18.0f, 1.5f, 1.5f, rng);
        break;

    case 1: // 3-B: ジグザグ茂み通路
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 9.0f, 3.0f, 5.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 15.0f, 10.0f, 4.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 14.0f, 3.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 16.0f, 3.5f, 2.5f, rng);
        break;

    case 2: // 3-C: 中央巨大ジャングル
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 13.0f, 8.0f, 7.0f, 4.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 6.0f, 2.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 8.0f, 16.0f, 3.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 18.0f, 4.0f, 1.5f, 1.5f, rng);
        break;

    case 3: // 3-D: 丸太迷路砦
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 8.0f, 5.0f, 4.0f, 2.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 15.0f, 13.0f, 2.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 12.0f, 17.0f, 5.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 6.0f, 3.0f, 3.0f, rng);
        break;
    }
}

// ----------------------------------------------------
// テーマ 4: 中央アメーバ大池 (4つのバリエーションステージ)
// ----------------------------------------------------
void StageGenerator::GenerateCenterLake(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng)
{
    switch (variation)
    {
    case 0: // 4-A: 巨大中央湖
        AddSymmetricOrganicCluster(stage, CellType::WATER, 18.0f, 9.0f, 4.5f, 3.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 9.0f, 6.0f, 3.5f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 8.0f, 12.0f, 5.5f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 12.0f, 19.0f, 1.5f, 1.5f, rng);
        break;

    case 1: // 4-B: 二重同心円水場
        AddSymmetricOrganicCluster(stage, CellType::WATER, 11.0f, 6.0f, 4.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 16.0f, 14.0f, 5.0f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 7.0f, 15.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 18.0f, 5.0f, 3.0f, 3.0f, rng);
        break;

    case 2: // 4-C: 三連アメーバ池
        AddSymmetricOrganicCluster(stage, CellType::WATER, 7.0f, 8.0f, 3.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 15.0f, 7.0f, 3.5f, 3.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WATER, 11.0f, 16.0f, 4.0f, 2.5f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 16.0f, 15.0f, 4.5f, 3.5f, rng);
        break;

    case 3: // 4-D: デルタ流木ゾーン
        AddSymmetricOrganicCluster(stage, CellType::WATER, 13.0f, 8.0f, 5.5f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::WALL_BLOCK, 6.0f, 9.0f, 3.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::BUSH, 18.0f, 8.0f, 4.0f, 4.0f, rng);
        AddSymmetricOrganicCluster(stage, CellType::CACTUS, 9.0f, 17.0f, 1.5f, 1.5f, rng);
        break;
    }
}
