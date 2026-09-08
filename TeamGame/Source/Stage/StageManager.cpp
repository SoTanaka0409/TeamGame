#include "StageManager.h"
#include "DxLib.h"
#include <ctime>
#include <vector>

StageManager::StageManager()
{
}

void StageManager::Initialize(int mapWidth, int mapHeight)
{
    m_config.mapWidth = mapWidth;
    m_config.mapHeight = mapHeight;

    // Grass1.png 画像の全方位検索ロード
    const std::vector<const char*> searchPaths = {
        "Resource/Grass1.png",
        "Resouce/Grass1.png",
        "Grass1.png",
        "TeamGame/Resource/Grass1.png",
        "TeamGame/Resouce/Grass1.png",
        "../Resource/Grass1.png",
        "../Resouce/Grass1.png",
        "c:/Users/student/Desktop/team/Resource/Grass1.png",
        "c:/Users/student/Desktop/team/Resouce/Grass1.png"
    };

    m_hGrassGraph = -1;
    for (const auto& path : searchPaths)
    {
        m_hGrassGraph = LoadGraph(path);
        if (m_hGrassGraph != -1) break;
    }

    // 起動時の初期乱数・テーマ・バリエーション
    m_currentSeed = m_rd() ^ static_cast<unsigned int>(std::time(nullptr));
    std::mt19937 initRng(m_currentSeed);

    int rawTheme = std::abs(static_cast<int>(initRng())) % 4;
    int rawVar   = std::abs(static_cast<int>(initRng())) % 4;

    m_currentTheme = static_cast<ThemePattern>(rawTheme);
    m_currentVariation = rawVar;

    m_config.theme = m_currentTheme;
    m_config.variation = m_currentVariation;

    Regenerate(m_currentSeed);
}

void StageManager::NextVariation()
{
    m_currentVariation = (m_currentVariation + 1) % 4;
    m_config.variation = m_currentVariation;
    Regenerate();
}

void StageManager::NextTheme()
{
    int nextThemeIdx = (static_cast<int>(m_currentTheme) + 1) % 4;
    m_currentTheme = static_cast<ThemePattern>(nextThemeIdx);
    m_currentVariation = 0;

    m_config.theme = m_currentTheme;
    m_config.variation = m_currentVariation;

    Regenerate();
}

void StageManager::Regenerate(unsigned int newSeed)
{
    if (newSeed == 0)
    {
        m_currentSeed = m_rd() ^ static_cast<unsigned int>(std::time(nullptr));
    }
    else
    {
        m_currentSeed = newSeed;
    }

    m_stage = StageGenerator::Generate(m_config, m_currentSeed, &m_currentTheme, &m_currentVariation);
    if (m_hGrassGraph != -1)
    {
        m_stage.SetGrassGraph(m_hGrassGraph);
    }
}

std::string StageManager::GetCurrentStageName() const
{
    return StageGenerator::GetFullStageName(m_currentTheme, m_currentVariation);
}

std::string StageManager::GetThemeName() const
{
    return StageGenerator::GetThemeName(m_currentTheme);
}

void StageManager::Draw(const Player& player, bool isDebugMode, int screenWidth, int screenHeight)
{
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);

    std::string stageName = GetCurrentStageName();

    // 1. Stageクラスの背景描画 (地形・水場・木箱・Grass1.png草むら)
    m_stage.DrawFitToArea(0, 0, screenWidth, screenHeight, isDebugMode, player.GetX(), player.GetY(), player.GetLightAngle(), stageName.c_str(), m_hGrassGraph);

    // スケール計算
    float cellW = static_cast<float>(screenWidth) / m_stage.GetWidth();
    float cellH = static_cast<float>(screenHeight) / m_stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;

    float mapPixelWidth = m_stage.GetWidth() * cellSize;
    float mapPixelHeight = m_stage.GetHeight() * cellSize;
    float startDrawX = (screenWidth - mapPixelWidth) / 2.0f;
    float startDrawY = (screenHeight - mapPixelHeight) / 2.0f;

    // 2. 【通常ホラー暗闇モード時】 レイキャスティング壁遮光・影付きライトマスク描画
    if (!isDebugMode)
    {
        player.RenderLightMask(m_stage, 0, 0, screenWidth, screenHeight, startDrawX, startDrawY, cellSize);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);

    // 3. Player本体の描画 (草むら潜伏演出対応)
    player.Draw(startDrawX, startDrawY, cellSize, isDebugMode);
}
