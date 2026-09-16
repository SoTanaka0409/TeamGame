#include "GameScene.h"
#include "Camera.h"
#include "DebugManager.h"
#include "EffectManager.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "ClearScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "PacketTypes.h"
#include "GameSettings.h"
#include <algorithm>
#include <cmath>
#include <random>

GameScene::GameScene(PlayMode mode) : player(nullptr), currentPlayMode(mode)
{
}

GameScene::~GameScene()
{
}

int GameScene::GetActiveEnemyCount() const
{
    if (!objectManager) return 0;
    int count = 0;
    for (auto obj : objectManager->GetObjects())
    {
        if (obj && obj->GetObjectTag() == ObjectTag::Enemy && obj->IsActive())
        {
            count++;
        }
    }
    return count;
}

void GameScene::ClearEnemies()
{
    if (objectManager)
    {
        for (auto obj : objectManager->GetObjects())
        {
            if (obj && obj->GetObjectTag() == ObjectTag::Enemy)
            {
                obj->SetActive(false);
            }
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

    std::vector<Point2D> validCells;
    for (int y = 1; y < stage.GetHeight() - 1; ++y)
    {
        for (int x = 1; x < stage.GetWidth() - 1; ++x)
        {
            if (!stage.IsSolidWall(x, y) && stage.GetCell(x, y) != CellType::WATER)
            {
                validCells.push_back({x, y});
            }
        }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(validCells.begin(), validCells.end(), g);

    // Spawn 2 Ally Bots (Team 0)
    for (int i = 0; i < 2 && i < validCells.size(); ++i)
    {
        Enemy* allyBot = new Enemy((validCells[i].x + 0.5f) * cellSize, (validCells[i].y + 0.5f) * cellSize, 0);
        allyBot->SetStage(const_cast<Stage*>(&stageManager.GetCurrentStage()), cellSize);
        objectManager->AddObject(allyBot);
    }
    
    // Spawn 3 Enemy Bots (Team 1)
    for (int i = 2; i < 5 && i < validCells.size(); ++i)
    {
        Enemy* enemyBot = new Enemy((validCells[i].x + 0.5f) * cellSize, (validCells[i].y + 0.5f) * cellSize, 1);
        enemyBot->SetStage(const_cast<Stage*>(&stageManager.GetCurrentStage()), cellSize);
        objectManager->AddObject(enemyBot);
    }
}
void GameScene::Init()
{
    Scene::Init();
    gameTimer = 0.0f;
    isCleared = false;
    
    // StageManagerの初期化
    stageManager.Initialize(48, 27);
    
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

    if (currentPlayMode == PlayMode::LOCAL_COOP)
    {
        remotePlayer = new Player(startX + 50.0f, startY);
        remotePlayer->SetStage(stagePtr, cellSize);
        remotePlayer->SetInputType(PlayerInputType::GAMEPAD_1);
    }
    else if (currentPlayMode == PlayMode::NETWORK_HOST || currentPlayMode == PlayMode::NETWORK_CLIENT)
    {
        remotePlayer = new Player(startX, startY);
        remotePlayer->SetStage(stagePtr, cellSize);
        remotePlayer->SetRemote(true);
    }
    
    // 敵を水・壁・外枠を避けてプレイヤーから離れたランダム位置にスポーン
    SpawnEnemiesRandomly(5);
}

void GameScene::Update()
{
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0 || (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);

    if (state == GameState::PLAYING)
    {
        if (currEsc && !prevEsc)
        {
            state = GameState::PAUSED;
            pauseMenuCursor = 0;
        }
        else
        {
            gameTimer += 0.016f;


            Scene::Update();

            // Respawn and kill count logic
            if (objectManager)
            {
                for (auto obj : objectManager->GetObjects())
                {
                    Character* ch = dynamic_cast<Character*>(obj);
                    if (ch && !ch->IsActive())
                    {
                        // 死亡しているキャラクターのキルカウント
                        if (ch->teamId == 0) team1Kills++; // 味方が死んだら敵にポイント
                        else if (ch->teamId == 1) team0Kills++; // 敵が死んだら味方にポイント
                        
                        // リスポーン（ランダムな位置に復活）
                        const Stage& stage = stageManager.GetCurrentStage();
                        float cellW = 1920.0f / stage.GetWidth();
                        float cellH = 1080.0f / stage.GetHeight();
                        float cellSize = (cellW < cellH) ? cellW : cellH;
                        
                        int rx = 1 + std::rand() % (stage.GetWidth() - 2);
                        int ry = 1 + std::rand() % (stage.GetHeight() - 2);
                        while (stage.IsSolidWall(rx, ry)) {
                            rx = 1 + std::rand() % (stage.GetWidth() - 2);
                            ry = 1 + std::rand() % (stage.GetHeight() - 2);
                        }
                        
                        ch->SetPosition(Vector2((rx + 0.5f) * cellSize, (ry + 0.5f) * cellSize));
                        ch->status.Heal(ch->status.GetMaxHp());
                        ch->SetActive(true);
                    }
                }
            }

            // 勝敗判定 (10キル先取)
            if (!isCleared && (team0Kills >= 10 || team1Kills >= 10))
            {
                isCleared = true;
                
                if (team0Kills >= 10) {
                    ClearStats stats;
                    stats.clearTimeSec = gameTimer;
                    stats.defeatedEnemies = team0Kills;
                    stats.totalEnemies = 10;
                    stats.rankScore = 5000;
                    stats.rankName = "S";
                    SceneManager::GetInstance().ChangeScene(std::make_shared<ClearScene>(stats));
                } else {
                    SceneManager::GetInstance().ChangeScene(std::make_shared<ResultScene>());
                }
                return;
            }

            DebugManager::GetInstance().Update();

            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_R))
            {
                stageManager.NextVariation();
                if (player)
                {
                    const Stage& stage = stageManager.GetCurrentStage();
                    float cellW = 1920.0f / stage.GetWidth();
                    float cellH = 1080.0f / stage.GetHeight();
                    float cellSize = (cellW < cellH) ? cellW : cellH;
                    Point2D startGrid = stage.GetPlayerStartPos();
                    player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                    player->SetStage(const_cast<Stage*>(&stageManager.GetCurrentStage()), cellSize);
                    SpawnEnemiesRandomly(5);
                }
            }

            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_T))
            {
                stageManager.NextTheme();
                if (player)
                {
                    const Stage& stage = stageManager.GetCurrentStage();
                    float cellW = 1920.0f / stage.GetWidth();
                    float cellH = 1080.0f / stage.GetHeight();
                    float cellSize = (cellW < cellH) ? cellW : cellH;
                    Point2D startGrid = stage.GetPlayerStartPos();
                    player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                    player->SetStage(const_cast<Stage*>(&stageManager.GetCurrentStage()), cellSize);
                    SpawnEnemiesRandomly(5);
                }
            }

            if (NetworkManager::GetInstance().IsConnected() && player)
            {
                PacketPlayerState packet;
                packet.header.type = PacketType::PLAYER_STATE;
                packet.x = player->GetPosition().x;
                packet.y = player->GetPosition().y;
                packet.facingX = player->GetFacingDir().x;
                packet.facingY = player->GetFacingDir().y;
                packet.isLightOn = player->IsLightOn();
                packet.isInBush = player->IsInBush();
                packet.currentWeaponIndex = 0;
                
                NetworkManager::GetInstance().SendPacket(&packet, sizeof(packet));
            }
            
            ProcessNetworkPackets();
        }
    }
    else if (state == GameState::PAUSED)
    {
        if (currEsc && !prevEsc) state = GameState::PLAYING;
        if (currUp && !prevUp) pauseMenuCursor--;
        if (currDown && !prevDown) pauseMenuCursor++;
        if (pauseMenuCursor < 0) pauseMenuCursor = 3;
        if (pauseMenuCursor > 3) pauseMenuCursor = 0;

        if (currEnter && !prevEnter)
        {
            if (pauseMenuCursor == 0) state = GameState::PLAYING;
            else if (pauseMenuCursor == 1) { state = GameState::SETTINGS; settingsMenuCursor = 0; }
            else if (pauseMenuCursor == 2) SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
            else if (pauseMenuCursor == 3) PostQuitMessage(0);
        }
    }
    else if (state == GameState::SETTINGS)
    {
        if (currEsc && !prevEsc) state = GameState::PAUSED;
        if (currUp && !prevUp) settingsMenuCursor--;
        if (currDown && !prevDown) settingsMenuCursor++;
        if (settingsMenuCursor < 0) settingsMenuCursor = 4;
        if (settingsMenuCursor > 4) settingsMenuCursor = 0;

        if (currEnter && !prevEnter)
        {
            if (settingsMenuCursor == 0) GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            else if (settingsMenuCursor == 1) DebugManager::GetInstance().ToggleDebugMode();
            else if (settingsMenuCursor == 2) { stageManager.NextTheme(); }
            else if (settingsMenuCursor == 3) { stageManager.NextVariation(); }
            else if (settingsMenuCursor == 4) state = GameState::PAUSED;
        }
    }

    prevEsc = currEsc;
    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
}

void GameScene::Draw()
{
    bool isDebugView = DebugManager::GetInstance().IsDebugMode();
    const Stage& stage = stageManager.GetCurrentStage();
    std::string stageName = stageManager.GetCurrentStageName();
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float worldCellSize = (cellW < cellH) ? cellW : cellH;
    float zoomCellSize = 75.0f;

    float playerWorldX = 0.0f, playerWorldY = 0.0f;
    if (player && player->IsActive())
    {
        Vector2 pos = player->GetPosition();
        playerWorldX = pos.x;
        playerWorldY = pos.y;
    }

    if (currentPlayMode == PlayMode::LOCAL_COOP && remotePlayer && remotePlayer->IsActive())
    {
        // 画面分割 (1P - 左半分)
        SetDrawArea(0, 0, 1920 / 2, 1080);
        Camera::TargetWorldX = playerWorldX;
        Camera::TargetWorldY = playerWorldY;
        Camera::ScreenCenterX = 1920.0f / 4.0f;
        Camera::ScreenCenterY = 1080.0f / 2.0f;
        Camera::ZoomScale = zoomCellSize / worldCellSize;
        stage.DrawZoomCamera(Camera::TargetWorldX, Camera::TargetWorldY, zoomCellSize, worldCellSize, isDebugView, stageName.c_str(), -1);
        Scene::Draw();

        // 画面分割 (2P - 右半分)
        SetDrawArea(1920 / 2, 0, 1920, 1080);
        Vector2 p2Pos = remotePlayer->GetPosition();
        Camera::TargetWorldX = p2Pos.x;
        Camera::TargetWorldY = p2Pos.y;
        Camera::ScreenCenterX = 1920.0f * 0.75f;
        Camera::ScreenCenterY = 1080.0f / 2.0f;
        Camera::ZoomScale = zoomCellSize / worldCellSize;
        stage.DrawZoomCamera(Camera::TargetWorldX, Camera::TargetWorldY, zoomCellSize, worldCellSize, isDebugView, stageName.c_str(), -1);
        Scene::Draw();
        
        // 描画エリアリセット
        SetDrawArea(0, 0, 1920, 1080);
        
        // 分割線
        DrawLine(1920 / 2, 0, 1920 / 2, 1080, GetColor(255, 255, 255), 3);
    }
    else
    {
        // 1画面
        Camera::TargetWorldX = playerWorldX;
        Camera::TargetWorldY = playerWorldY;
        Camera::ScreenCenterX = 1920.0f / 2.0f;
        Camera::ScreenCenterY = 1080.0f / 2.0f;
        Camera::ZoomScale = zoomCellSize / worldCellSize;
        stage.DrawZoomCamera(playerWorldX, playerWorldY, zoomCellSize, worldCellSize, isDebugView, stageName.c_str(), -1);
        Scene::Draw();
    }

    if (!DebugManager::GetInstance().IsDebugMode() && player && player->IsActive())
    {
        player->RenderLightMask(0, 0, 1920, 1080, 0, 0);
    }
    
    DrawString(10, 10, "[ESC]キーでポーズ", GetColor(255, 255, 255));
    DrawString(10, 30, (std::string("Theme: ") + std::to_string((int)stageManager.GetCurrentTheme() + 1)).c_str(), GetColor(150, 150, 150));
    DrawString(10, 50, (std::string("Variation: ") + std::to_string(stageManager.GetCurrentVariation() + 1)).c_str(), GetColor(150, 150, 150));

    if (player && player->IsActive()) {
        player->DrawUI(10, 100);
    }
    if (currentPlayMode == PlayMode::LOCAL_COOP && remotePlayer && remotePlayer->IsActive()) {
        remotePlayer->DrawUI(1920 / 2 + 10, 100);
    }

    if (state == GameState::PAUSED || state == GameState::SETTINGS)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, 0, 1920, 1080, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawString(1920 / 2 - 50, 200, "== ポーズ ==", GetColor(255, 255, 0));

        const int menuStartX = 1920 / 2 - 100;
        const int menuStartY = 400;
        const int menuSpacing = 60;
        
    
    // Draw scores
    char scoreText[128];
    sprintf_s(scoreText, sizeof(scoreText), "BLUE(YOU): %d  vs  RED: %d", team0Kills, team1Kills);
    DrawString(800, 20, scoreText, GetColor(255, 255, 255));
    
    if (state == GameState::PAUSED)

        {
            const char* items[] = { "ゲームに戻る", "設定", "タイトルへ戻る", "ゲーム終了" };
            for (int i = 0; i < 4; i++)
            {
                unsigned int color = (i == pauseMenuCursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
                if (i == pauseMenuCursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
                DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color);
            }
        }
        else if (state == GameState::SETTINGS)
        {
            std::string items[] = { 
                std::string("視点固定モード (Eキー) : ") + (GameSettings::GetInstance().isAimLockHoldMode ? "長押し (ON)" : "切り替え (OFF)"),
                std::string("デバッグ表示 : ") + (isDebugView ? "ON" : "OFF"), 
                "テーマ変更", 
                "マップ変更", 
                "戻る" 
            };
            for (int i = 0; i < 5; i++)
            {
                unsigned int color = (i == settingsMenuCursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
                if (i == settingsMenuCursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
                DrawString(menuStartX, menuStartY + i * menuSpacing, items[i].c_str(), color);
            }
        }
        
        int ruleX = 1300;
        int ruleY = 300;
        DrawBox(ruleX - 20, ruleY - 20, 1850, 800, GetColor(30, 30, 40), TRUE);
        DrawBox(ruleX - 20, ruleY - 20, 1850, 800, GetColor(100, 100, 100), FALSE);
        DrawString(ruleX, ruleY, "【遊び方・操作】", GetColor(255, 200, 0));
        DrawString(ruleX, ruleY + 40, "W A S D : 移動", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 80, "マウス : 視点移動 / 狙う", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 120, "左クリック : 撃つ (または Z キー)", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 160, "右クリック : 懐中電灯ON/OFF", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 200, "Q キー : 武器切り替え", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 240, "E キー : 視点固定", GetColor(255, 255, 255));
        
        DrawString(ruleX, ruleY + 300, "[ ルール ]", GetColor(255, 200, 0));
        DrawString(ruleX, ruleY + 340, "- 敵の攻撃を避けながら進む", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 380, "- 草むらにいると敵から見えにくくなる", GetColor(255, 255, 255));
        DrawString(ruleX, ruleY + 420, "- ライトを消すとステルス性が上がる", GetColor(255, 255, 255));
    }

    int activeEnemyCount = GetActiveEnemyCount();
    
    // Draw an obvious Enemy counter in the top right corner
    char enemyText[64];
    snprintf(enemyText, sizeof(enemyText), "Enemies: %d", activeEnemyCount);
    DrawString(1920 - 200, 20, enemyText, GetColor(255, 100, 100));

    DebugManager::GetInstance().DrawDebugOverlay(stageName, playerWorldX, playerWorldY, activeEnemyCount);
}

void GameScene::ProcessNetworkPackets()
{
    auto packets = NetworkManager::GetInstance().ReceivePackets();
    for (const auto& data : packets)
    {
        if (data.size() < sizeof(PacketHeader)) continue;
        
        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data.data());
        
        if (header->type == PacketType::STAGE_INIT && !NetworkManager::GetInstance().IsHost())
        {
            const PacketStageInit* packet = reinterpret_cast<const PacketStageInit*>(data.data());
            
            stageManager.Regenerate(packet->seed);
            
            const Stage& stage = stageManager.GetCurrentStage();
            float cellW = 1920.0f / stage.GetWidth();
            float cellH = 1080.0f / stage.GetHeight();
            float cellSize = (cellW < cellH) ? cellW : cellH;
            
            if (player) {
                Point2D startGrid = stage.GetPlayerStartPos();
                player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                player->SetStage(const_cast<Stage*>(&stage), cellSize);
            }
            SpawnEnemiesRandomly(5);
        }
        else if (header->type == PacketType::PLAYER_STATE && remotePlayer)
        {
            const PacketPlayerState* packet = reinterpret_cast<const PacketPlayerState*>(data.data());
            remotePlayer->SetPosition(Vector2(packet->x, packet->y));
            remotePlayer->SetFacingDir(Vector2(packet->facingX, packet->facingY));
            if (packet->isLightOn && !remotePlayer->IsLightOn()) remotePlayer->ToggleLight();
            if (!packet->isLightOn && remotePlayer->IsLightOn()) remotePlayer->ToggleLight();
            remotePlayer->SetInBush(packet->isInBush);
        }
    }
}
