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
        Vector2 pos = player->GetPosition();
        Vector2 dir = player->GetFacingDir();

        float facingAngle = std::atan2(dir.y, dir.x);
        float fov = 90.0f * (3.14159265f / 180.0f);

        // --- 1. ライトの範囲（グレー半透明）を描画 ---
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        float lightStart = facingAngle - fov / 2.0f;
        float lightStep = fov / 16.0f;
        float lightDist = 600.0f; // ライトが届く距離

        for (int i = 0; i < 16; ++i)
        {
            float a1 = lightStart + lightStep * i;
            float a2 = lightStart + lightStep * (i + 1);
            int x1 = static_cast<int>(pos.x + std::cos(a1) * lightDist);
            int y1 = static_cast<int>(pos.y + std::sin(a1) * lightDist);
            int x2 = static_cast<int>(pos.x + std::cos(a2) * lightDist);
            int y2 = static_cast<int>(pos.y + std::sin(a2) * lightDist);
            DrawTriangle(static_cast<int>(pos.x), static_cast<int>(pos.y), x1,
                         y1, x2, y2, GetColor(200, 200, 200), TRUE);
        }

        // --- 2. 視界以外の暗闇を描画 ---
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 240); // ほとんど真っ暗

        // 横と後ろの暗闇
        float angleStart = facingAngle + fov / 2.0f;
        float angleEnd = facingAngle + 2.0f * 3.14159265f - fov / 2.0f;
        int steps = 24;
        float angleStep = (angleEnd - angleStart) / steps;

        for (int i = 0; i < steps; ++i)
        {
            float a1 = angleStart + angleStep * i;
            float a2 = angleStart + angleStep * (i + 1);
            float dist = 3000.0f;
            int x1 = static_cast<int>(pos.x + std::cos(a1) * dist);
            int y1 = static_cast<int>(pos.y + std::sin(a1) * dist);
            int x2 = static_cast<int>(pos.x + std::cos(a2) * dist);
            int y2 = static_cast<int>(pos.y + std::sin(a2) * dist);
            DrawTriangle(static_cast<int>(pos.x), static_cast<int>(pos.y), x1,
                         y1, x2, y2, GetColor(0, 0, 0), TRUE);
        }

        // 正面だけどライトが届かない奥の暗闇
        for (int i = 0; i < 16; ++i)
        {
            float a1 = lightStart + lightStep * i;
            float a2 = lightStart + lightStep * (i + 1);
            float distFar = 3000.0f;
            int x1 = static_cast<int>(pos.x + std::cos(a1) * lightDist);
            int y1 = static_cast<int>(pos.y + std::sin(a1) * lightDist);
            int x2 = static_cast<int>(pos.x + std::cos(a2) * lightDist);
            int y2 = static_cast<int>(pos.y + std::sin(a2) * lightDist);

            int x3 = static_cast<int>(pos.x + std::cos(a1) * distFar);
            int y3 = static_cast<int>(pos.y + std::sin(a1) * distFar);
            int x4 = static_cast<int>(pos.x + std::cos(a2) * distFar);
            int y4 = static_cast<int>(pos.y + std::sin(a2) * distFar);

            DrawQuadrangle(x1, y1, x2, y2, x4, y4, x3, y3, GetColor(0, 0, 0),
                           TRUE);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}
