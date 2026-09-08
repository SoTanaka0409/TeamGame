#pragma once

#include "Stage.h"
#include "StageGenerator.h"
#include "Player.h"
#include <string>
#include <random>

class StageManager
{
public:
    StageManager();
    ~StageManager() = default;

    // ステージマネージャーの初期化・リソース読込
    void Initialize(int mapWidth = 48, int mapHeight = 27);

    // 次のステージバリエーション (Rキー: 1〜4)
    void NextVariation();

    // 次のテーマデザイン (Tキー: 1〜4)
    void NextTheme();

    // 新しいシード値で再生成
    void Regenerate(unsigned int newSeed = 0);

    // ステージ背景描画 ＋ Playerのライトマスク ＋ Player本体の統合描画
    void Draw(const Player& player, bool isDebugMode, int screenWidth, int screenHeight);

    // ゲッター
    const Stage& GetCurrentStage() const { return m_stage; }
    ThemePattern GetCurrentTheme() const { return m_currentTheme; }
    int GetCurrentVariation() const { return m_currentVariation; }
    unsigned int GetCurrentSeed() const { return m_currentSeed; }

    std::string GetCurrentStageName() const;
    std::string GetThemeName() const;

private:
    StageGenConfig m_config;
    Stage m_stage;

    ThemePattern m_currentTheme = ThemePattern::EMERALD_MEADOW;
    int m_currentVariation = 0;
    unsigned int m_currentSeed = 0;

    int m_hGrassGraph = -1; // Grass1.png 画像ハンドル
    std::random_device m_rd;
};
