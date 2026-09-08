#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "SceneManager.h"

GameScene::GameScene() : player(nullptr)
{
}
GameScene::~GameScene()
{
}

#include "StageGenerator.h"
#include <random>
#include <ctime>

void GameScene::Init()
{
    Scene::Init();
    
    // ステージ生成
    std::random_device rd;
    unsigned int seed = rd() ^ static_cast<unsigned int>(std::time(nullptr));
    StageGenConfig config;
    themeIdx = seed % 4;
    varIdx = seed % 4;
    config.theme = static_cast<ThemePattern>(themeIdx);
    config.variation = varIdx;
    
    ThemePattern currTheme = config.theme;
    int currVar = config.variation;
    
    stage = StageGenerator::Generate(config, seed, &currTheme, &currVar);
    
    // セルサイズ計算 (1920x1080画面に合わせる)
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    Point2D startGrid = stage.GetPlayerStartPos();
    float startX = (startGrid.x + 0.5f) * cellSize;
    float startY = (startGrid.y + 0.5f) * cellSize;
    
    player = new Player(startX, startY);
    player->SetStage(&stage, cellSize);
    
    // 敵のスポーン位置も追加できるが、とりあえず固定位置に1体
    new Enemy(startX + 200.0f, startY + 200.0f);
}

void GameScene::Update()
{
    Scene::Update(); // 自身の持つobjectManagerやcolliderManagerが実行される

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(
            std::make_shared<ResultScene>());
    }
}

#include <cmath>

void GameScene::Draw()
{
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    float playerGridX = 0;
    float playerGridY = 0;
    float facingAngle = 0;
    
    if (player && player->IsActive())
    {
        Vector2 pos = player->GetPosition();
        Vector2 dir = player->GetFacingDir();
        playerGridX = pos.x / cellSize;
        playerGridY = pos.y / cellSize;
        facingAngle = std::atan2(dir.y, dir.x);
    }
    
    // ステージ描画
    std::string stageName = StageGenerator::GetFullStageName(static_cast<ThemePattern>(themeIdx), varIdx);
    stage.DrawFitToArea(0, 0, 1920, 1080, true, playerGridX, playerGridY, facingAngle, stageName.c_str(), -1);

    Scene::Draw();

    if (player && player->IsActive())
    {
        // プレイヤー自身の描画は Scene::Draw() の中(objectManager)で行われているため、
        // ここでは暗闇とライト（マスク）のみを描画する
        
        // 描画エリアは 1920x1080
        float mapPixelWidth = stage.GetWidth() * cellSize;
        float mapPixelHeight = stage.GetHeight() * cellSize;
        float startDrawX = (1920 - mapPixelWidth) / 2.0f;
        float startDrawY = (1080 - mapPixelHeight) / 2.0f;

        // 【eitaの暗闇・ライト機能】
        player->RenderLightMask(0, 0, 1920, 1080, startDrawX, startDrawY);
    }
}
