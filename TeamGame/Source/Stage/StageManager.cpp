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

    // 画像ファイルの全方位検索ロード用ヘルパー
    auto loadTileGraph = [](const std::vector<const char*>& fileNames) -> int {
        const std::vector<const char*> baseDirs = {
            "Resource/",
            "Resouce/",
            "",
            "TeamGame/Resource/",
            "TeamGame/Resouce/",
            "../Resource/",
            "../Resouce/",
            "c:/Users/student/Desktop/team/Resource/",
            "c:/Users/student/Desktop/team/Resouce/"
        };
        for (const auto& fileName : fileNames)
        {
            for (const auto& dir : baseDirs)
            {
                std::string fullPath = std::string(dir) + fileName;
                int handle = LoadGraph(fullPath.c_str());
                if (handle != -1) return handle;
            }
        }
        return -1;
    };

    m_hFloorGraph     = loadTileGraph({ "Floor.png", "Floor1.png", "floor.png" });
    m_hWallBlockGraph = loadTileGraph({ "BigRock.png", "RockLarge.png", "Rock_Large.png", "Big_Rock.png", "Wall.png", "Wall1.png", "WallBlock.png", "wall.png" });
    m_hOuterWallGraph = loadTileGraph({ "OuterWall.png", "OuterWall1.png", "outerwall.png" });
    m_hGrassGraph     = loadTileGraph({ "Grass1.png", "Grass.png", "grass.png" });
    m_hWaterGraph     = loadTileGraph({ "Water.png", "Water1.png", "water.png" });
    m_hCactusGraph    = loadTileGraph({ "Rock.png", "SmallRock.png", "NormalRock.png", "RockNormal.png", "Rock_Small.png", "Cactus.png", "cactus.png" });
    m_hStarGraph      = loadTileGraph({ "Star.png", "Star1.png", "star.png" });

    // 起動時の初期乱数・テーマ・バリエーション
    m_currentSeed = m_rd() ^ static_cast<unsigned int>(std::time(nullptr));
    std::mt19937 initRng(m_currentSeed);

    // 起動時の初期テーマ・バリエーション (ユーザー作成のカスタムマップをデフォルトに設定)
    m_currentTheme = ThemePattern::CENTER_LAKE;
    m_currentVariation = 0;

    m_config.theme = m_currentTheme;
    m_config.variation = m_currentVariation;

    Regenerate(m_currentSeed);
}

void StageManager::NextVariation()
{
    m_currentTheme = ThemePattern::CENTER_LAKE;
    m_currentVariation = 0;
    m_config.theme = m_currentTheme;
    m_config.variation = m_currentVariation;
    Regenerate();
}

void StageManager::NextTheme()
{
    m_currentTheme = ThemePattern::CENTER_LAKE;
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
    
    m_stage.SetFloorGraph(m_hFloorGraph);
    m_stage.SetWallBlockGraph(m_hWallBlockGraph);
    m_stage.SetOuterWallGraph(m_hOuterWallGraph);
    m_stage.SetGrassGraph(m_hGrassGraph);
    m_stage.SetWaterGraph(m_hWaterGraph);
    m_stage.SetCactusGraph(m_hCactusGraph);
    m_stage.SetStarGraph(m_hStarGraph);
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

    // スケール計算
    float cellW = static_cast<float>(screenWidth) / m_stage.GetWidth();
    float cellH = static_cast<float>(screenHeight) / m_stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;

    Vector2 pos = player.GetPosition();
    Vector2 dir = player.GetFacingDir();
    float playerGridX = pos.x / cellSize;
    float playerGridY = pos.y / cellSize;
    float lightAngle = std::atan2(dir.y, dir.x);

    // 1. Stageクラスの背景描画 (地形・水場・木箱・Grass1.png草むら)
    m_stage.DrawFitToArea(0, 0, screenWidth, screenHeight, isDebugMode, playerGridX, playerGridY, lightAngle, stageName.c_str(), m_hGrassGraph);

    float mapPixelWidth = m_stage.GetWidth() * cellSize;
    float mapPixelHeight = m_stage.GetHeight() * cellSize;
    float startDrawX = (screenWidth - mapPixelWidth) / 2.0f;
    float startDrawY = (screenHeight - mapPixelHeight) / 2.0f;

    // 2. 【通常ホラー暗闇モード時】 レイキャスティング壁遮光・影付きライトマスク描画
    if (!isDebugMode)
    {
        player.RenderLightMask(0, 0, screenWidth, screenHeight, startDrawX, startDrawY);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);
}
