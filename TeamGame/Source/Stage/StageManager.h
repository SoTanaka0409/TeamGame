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

    int m_hFloorGraph = -1;     // 床画像
    int m_hWallBlockGraph = -1; // 壁・木箱画像
    int m_hOuterWallGraph = -1; // 外枠壁画像
    int m_hGrassGraph = -1;     // 草むら画像
    int m_hGrassModel = -1;     // 3D草むらモデルハンドル (grass.mv1)
    float m_grassModelBaseSize = 1.2f; // 3D草むらモデルの自動計測ベースサイズ
    int m_hRockModels[4] = { -1, -1, -1, -1 }; // 3D岩モデルハンドル (rock_01〜04.mv1)
    int m_hRockTextures[4] = { -1, -1, -1, -1 }; // 3D岩テクスチャハンドル (rock_01〜04_diffuse.png)
    float m_rockModelBaseSizes[4] = { 5.0f, 5.0f, 5.0f, 5.0f }; // 3D岩モデルの自動計測水平サイズ
    int m_hWaterGraph = -1;     // 水場画像
    int m_hCactusGraph = -1;    // サボテン・岩画像
    int m_hStarGraph = -1;      // 中央スター画像

    std::random_device m_rd;
};
