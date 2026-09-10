#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "SceneManager.h"
#include <algorithm>
#include <cmath>
#include <random>

GameScene::GameScene() : player(nullptr)
{
}

GameScene::~GameScene()
{
}

void GameScene::ClearEnemies()
{
    for (auto e : enemies)
    {
        if (e)
        {
            e->SetActive(false);
        }
    }
    enemies.clear();
}

void GameScene::SpawnEnemiesRandomly(int count)
{
    ClearEnemies();

    const Stage& stage = stageManager.GetCurrentStage();
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    Point2D pStart = stage.GetPlayerStartPos();

    std::vector<Point2D> validCells;
    for (int y = 1; y < stage.GetHeight() - 1; ++y)
    {
        for (int x = 1; x < stage.GetWidth() - 1; ++x)
        {
            CellType cell = stage.GetCell(x, y);
            // 水(WATER)、障害物壁(WALL_BLOCK, CACTUS)、外枠(OUTER_WALL)を除外し、床と草むらのみ対象
            if (cell == CellType::EMPTY_FLOOR || cell == CellType::BUSH)
            {
                if (!stage.IsSolidWall(x, y) && cell != CellType::WATER)
                {
                    // プレイヤー初期位置から一定距離(5マス以上)を離してスポーン
                    int dx = x - pStart.x;
                    int dy = y - pStart.y;
                    if ((dx * dx + dy * dy) >= 25)
                    {
                        validCells.push_back({ x, y });
                    }
                }
            }
        }
    }

    if (validCells.empty()) return;

    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(validCells.begin(), validCells.end(), rng);

    Stage* stagePtr = const_cast<Stage*>(&stageManager.GetCurrentStage());
    int numToSpawn = (std::min)(count, static_cast<int>(validCells.size()));

    for (int i = 0; i < numToSpawn; ++i)
    {
        float ex = (validCells[i].x + 0.5f) * cellSize;
        float ey = (validCells[i].y + 0.5f) * cellSize;
        Enemy* enemy = new Enemy(ex, ey);
        enemy->SetStage(stagePtr, cellSize);
        enemy->SetTargetPlayer(player);
        enemies.push_back(enemy);
    }
}

void GameScene::Init()
{
    Scene::Init();
    
    // StageManagerの初期化
    stageManager.Initialize(48, 27);
    DebugManager::GetInstance().SetDebugMode(false); // 最初から暗闇モード
    
    const Stage& stage = stageManager.GetCurrentStage();
    
    // セルサイズ計算 (1920x1080画面に合わせる)
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    Point2D startGrid = stage.GetPlayerStartPos();
    float startX = (startGrid.x + 0.5f) * cellSize;
    float startY = (startGrid.y + 0.5f) * cellSize;
    
    player = new Player(startX, startY);
    Stage* stagePtr = const_cast<Stage*>(&stageManager.GetCurrentStage());
    player->SetStage(stagePtr, cellSize);
    
    // 敵を水・壁・外枠を避けてプレイヤーから離れたランダム位置にスポーン
    SpawnEnemiesRandomly(5);
}

void GameScene::Update()
{
    Scene::Update(); // 自身の持つobjectManagerやcolliderManagerが実行される
    DebugManager::GetInstance().Update(); // デバッグショートカットキー処理

    // Rキーで位置のリセット (ステージを変更せずにプレイヤーと敵の位置を初期化)
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_R))
    {
        if (player)
        {
            const Stage& stage = stageManager.GetCurrentStage();
            float cellW = 1920.0f / stage.GetWidth();
            float cellH = 1080.0f / stage.GetHeight();
            float cellSize = (cellW < cellH) ? cellW : cellH;
            Point2D startGrid = stage.GetPlayerStartPos();
            player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
            SpawnEnemiesRandomly(5);
        }
    }
}

void GameScene::Draw()
{
    const Stage& stage = stageManager.GetCurrentStage();
    std::string stageName = stageManager.GetCurrentStageName();
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float worldCellSize = (cellW < cellH) ? cellW : cellH;
    float zoomCellSize = 75.0f; // プレイヤー中心ズームのセルサイズ

    float playerWorldX = 0.0f, playerWorldY = 0.0f;
    if (player && player->IsActive())
    {
        Vector2 pos = player->GetPosition();
        playerWorldX = pos.x;
        playerWorldY = pos.y;
    }

    bool isDebug = DebugManager::GetInstance().IsDebugMode();

    // 1. プレイヤーの位置(playerWorldX, playerWorldY)を 1920x1080 画面中央 (960, 540) に配置するズームカメラ描画
    stage.DrawZoomCamera(playerWorldX, playerWorldY, zoomCellSize, worldCellSize, isDebug, stageName.c_str(), -1);

    // 2. オブジェクト類 (プレイヤー・敵・弾丸) のカメラ相対描画
    Scene::Draw();

    // 3. プレイヤーを中心とするホラー暗闇スポットライトマスクの描画 (非デバッグ表示時)
    if (!isDebug && player && player->IsActive())
    {
        player->RenderLightMask(0, 0, 1920, 1080, 0, 0);
    }

    // 4. 専用デバッグオーバーレイの描画
    int activeEnemyCount = 0;
    for (auto e : enemies)
    {
        if (e && e->IsActive()) activeEnemyCount++;
    }
    DebugManager::GetInstance().DrawDebugOverlay(stageName, playerWorldX, playerWorldY, activeEnemyCount);
}
