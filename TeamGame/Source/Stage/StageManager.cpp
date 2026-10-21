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

    // 3D草むらモデル (grass.mv1) の読み込みとマテリアル明度設定
    if (m_hGrassModel == -1)
    {
        const std::vector<const char*> grassModelPaths = {
            "Resource/grass_02_plain_dark.mv1",
            "Resource/Nature/01_草/grass_02_plain_dark.mv1",
            "TeamGame/Resource/grass_02_plain_dark.mv1",
            "TeamGame/Resource/Nature/01_草/grass_02_plain_dark.mv1",
            "../Resource/grass_02_plain_dark.mv1",
            "../Resource/Nature/01_草/grass_02_plain_dark.mv1",
            "c:/Users/student/Desktop/3D/自然物/01_草/grass_02_plain_dark.mv1",
            "Resource/grass.mv1",
            "Resource/Nature/01_草/grass.mv1"
        };
        for (const char* path : grassModelPaths)
        {
            m_hGrassModel = MV1LoadModel(path);
            if (m_hGrassModel != -1) break;
        }
        if (m_hGrassModel != -1)
        {
            // バウンディングボックスからモデルの正確な基準サイズを自動計測
            MV1SetupReferenceMesh(m_hGrassModel, -1, TRUE);
            MV1_REF_POLYGONLIST refMesh = MV1GetReferenceMesh(m_hGrassModel, -1, TRUE);
            if (refMesh.VertexNum > 0)
            {
                float sx = refMesh.MaxPosition.x - refMesh.MinPosition.x;
                float sy = refMesh.MaxPosition.y - refMesh.MinPosition.y;
                float sz = refMesh.MaxPosition.z - refMesh.MinPosition.z;
                float maxSpan = sx;
                if (sz > maxSpan) maxSpan = sz;
                if (sy > maxSpan) maxSpan = sy;
                if (maxSpan > 0.001f)
                {
                    m_grassModelBaseSize = maxSpan;
                }
            }

            int matNum = MV1GetMaterialNum(m_hGrassModel);
            for (int m = 0; m < matNum; ++m)
            {
                COLOR_F diff = MV1GetMaterialDifColor(m_hGrassModel, m);
                MV1SetMaterialAmbColor(m_hGrassModel, m, GetColorF(diff.r * 0.9f, diff.g * 0.9f, diff.b * 0.9f, 1.0f));
                MV1SetMaterialSpcColor(m_hGrassModel, m, GetColorF(0.25f, 0.35f, 0.20f, 1.0f));
            }

            int meshNum = MV1GetMeshNum(m_hGrassModel);
            for (int i = 0; i < meshNum; ++i)
            {
                // 草の葉の裏面も見えるようにバックカリングを無効化
                MV1SetMeshBackCulling(m_hGrassModel, i, DX_CULLING_NONE);
            }
        }
    }

    // 3D岩モデル4種 (rock_01〜04.mv1) とテクスチャ (rock_01〜04_diffuse.png) の読み込み
    const char* rockModelNames[4] = { "rock_01.mv1", "rock_02.mv1", "rock_03.mv1", "rock_04.mv1" };
    const char* rockTexNames[4]   = { "rock_01_diffuse.png", "rock_02_diffuse.png", "rock_03_diffuse.png", "rock_04_diffuse.png" };
    const std::vector<const char*> modelSearchDirs = {
        "Resource/",
        "Resource/Nature/02_岩/",
        "TeamGame/Resource/",
        "TeamGame/Resource/Nature/02_岩/",
        "../Resource/",
        "../Resource/Nature/02_岩/",
        "c:/Users/student/Desktop/3D/自然物/02_岩/"
    };

    for (int r = 0; r < 4; ++r)
    {
        if (m_hRockModels[r] == -1)
        {
            for (const char* dir : modelSearchDirs)
            {
                std::string fullPath = std::string(dir) + rockModelNames[r];
                m_hRockModels[r] = MV1LoadModel(fullPath.c_str());
                if (m_hRockModels[r] != -1) break;
            }
        }
        if (m_hRockTextures[r] == -1)
        {
            m_hRockTextures[r] = loadTileGraph({ rockTexNames[r] });
        }
        if (m_hRockModels[r] != -1)
        {
            if (m_hRockTextures[r] != -1)
            {
                MV1SetTextureGraphHandle(m_hRockModels[r], 0, m_hRockTextures[r], FALSE);
            }

            MV1SetupReferenceMesh(m_hRockModels[r], -1, TRUE);
            MV1_REF_POLYGONLIST refMesh = MV1GetReferenceMesh(m_hRockModels[r], -1, TRUE);
            if (refMesh.VertexNum > 0)
            {
                float sx = refMesh.MaxPosition.x - refMesh.MinPosition.x;
                float sz = refMesh.MaxPosition.z - refMesh.MinPosition.z;
                float horizSpan = (sx > sz) ? sx : sz;
                if (horizSpan > 0.001f)
                {
                    m_rockModelBaseSizes[r] = horizSpan;
                }
            }

            int matNum = MV1GetMaterialNum(m_hRockModels[r]);
            for (int m = 0; m < matNum; ++m)
            {
                MV1SetMaterialDifColor(m_hRockModels[r], m, GetColorF(0.95f, 0.95f, 0.95f, 1.0f));
                MV1SetMaterialAmbColor(m_hRockModels[r], m, GetColorF(0.80f, 0.82f, 0.80f, 1.0f));
                MV1SetMaterialSpcColor(m_hRockModels[r], m, GetColorF(0.18f, 0.18f, 0.18f, 1.0f));
            }
        }
    }

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
    m_stage.SetGrassModel(m_hGrassModel, m_grassModelBaseSize);
    for (int r = 0; r < 4; ++r)
    {
        m_stage.SetRockModel(r, m_hRockModels[r], m_rockModelBaseSizes[r]);
    }
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
