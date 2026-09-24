#pragma once

#include "Stage.h"
#include <random>
#include <string>

// 4つのテーマ（デザインパターン）
enum class ThemePattern
{
    EMERALD_MEADOW = 0, // テーマ1: エメラルド草原
    CROSS_ROADS,        // テーマ2: 十字路 & 水場砦
    SIDE_FORESTS,       // テーマ3: サイド茂み & 丸太砦
    CENTER_LAKE         // テーマ4: 中央アメーバ大池
};

struct StageGenConfig
{
    int mapWidth = 48;                 // 横セル数
    int mapHeight = 27;                // 縦セル数
    ThemePattern theme = ThemePattern::EMERALD_MEADOW; // 4つのデザインテーマ
    int variation = 0;                 // テーマ内の4つのステージバリエーション (0〜3)
};

class StageGenerator
{
public:
    StageGenerator() = default;

    // 4つのテーマデザイン × 各4つのステージバリエーション（計16パターン）生成
    static Stage Generate(const StageGenConfig& config, unsigned int seed = 0, ThemePattern* outTheme = nullptr, int* outVariation = nullptr);

    static std::string GetThemeName(ThemePattern theme);
    static std::string GetFullStageName(ThemePattern theme, int variation);

private:
    // 各テーマごとの4バリエーション生成関数
    static void GenerateEmeraldMeadow(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng);
    static void GenerateCrossRoads(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng);
    static void GenerateSideForests(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng);
    static void GenerateCenterLake(Stage& stage, int variation, const StageGenConfig& config, std::mt19937& rng);

    // 有機的・自然形状クラスター配置
    static void AddSymmetricOrganicCluster(Stage& stage, CellType type, float cx, float cy, float radiusX, float radiusY, std::mt19937& rng);
};
