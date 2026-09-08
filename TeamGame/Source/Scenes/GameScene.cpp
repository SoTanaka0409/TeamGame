#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "GameSettings.h"

GameScene::GameScene() : player(nullptr)
{
}
GameScene::~GameScene()
{
}

#include "NetworkManager.h"
#include "PacketTypes.h"
#include "StageGenerator.h"
#include <random>
#include <ctime>

void GameScene::Init()
{
    Scene::Init();
    
    // ステージ生成
    std::random_device rd;
    unsigned int seed = rd() ^ static_cast<unsigned int>(std::time(nullptr));
    
    // ホストならランダムに決定、クライアントなら最初は適当で後で受信
    themeIdx = seed % 4;
    varIdx = seed % 4;
    
    StageGenConfig config;
    config.theme = static_cast<ThemePattern>(themeIdx);
    config.variation = varIdx;
    
    ThemePattern currTheme = config.theme;
    int currVar = config.variation;
    
    stage = StageGenerator::Generate(config, seed, &currTheme, &currVar);
    
    // セルサイズ計算
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    Point2D startGrid = stage.GetPlayerStartPos();
    float startX = (startGrid.x + 0.5f) * cellSize;
    float startY = (startGrid.y + 0.5f) * cellSize;
    
    player = new Player(startX, startY);
    player->SetStage(&stage, cellSize);
    
    // リモートプレイヤーの生成（マルチプレイ時のみ）
    bool isMultiplayer = NetworkManager::GetInstance().IsConnected();
    bool isHostOrSingle = !isMultiplayer || NetworkManager::GetInstance().IsHost();

    if (isMultiplayer)
    {
        remotePlayer = new Player(startX, startY);
        remotePlayer->SetStage(&stage, cellSize);
        remotePlayer->SetRemote(true);
    }
    else
    {
        remotePlayer = nullptr;
    }
    
    // ホストまたはシングルプレイならステージ情報送信や敵生成を行う
    if (isHostOrSingle)
    {
        if (isMultiplayer)
        {
            PacketStageInit packet;
            packet.header.type = PacketType::STAGE_INIT;
            packet.seed = seed;
            packet.themeIdx = themeIdx;
            packet.varIdx = varIdx;
            NetworkManager::GetInstance().SendPacket(&packet, sizeof(packet));
        }
        
        // 敵の生成
        new Enemy(startX + 200.0f, startY + 200.0f);
    }
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
            
            themeIdx = packet->themeIdx;
            varIdx = packet->varIdx;
            
            StageGenConfig config;
            config.theme = static_cast<ThemePattern>(themeIdx);
            config.variation = varIdx;
            ThemePattern currT = config.theme;
            int currV = config.variation;
            
            stage = StageGenerator::Generate(config, packet->seed, &currT, &currV);
            
            float cellW = 1920.0f / stage.GetWidth();
            float cellH = 1080.0f / stage.GetHeight();
            float cellSize = (cellW < cellH) ? cellW : cellH;
            
            if (player) {
                Point2D startGrid = stage.GetPlayerStartPos();
                player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                player->SetStage(&stage, cellSize);
            }
            if (remotePlayer) {
                Point2D startGrid = stage.GetPlayerStartPos();
                remotePlayer->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                remotePlayer->SetStage(&stage, cellSize);
            }
        }
        else if (header->type == PacketType::PLAYER_STATE)
        {
            const PacketPlayerState* packet = reinterpret_cast<const PacketPlayerState*>(data.data());
            if (remotePlayer)
            {
                remotePlayer->SetPosition(Vector2(packet->x, packet->y));
                remotePlayer->SetFacingDir(Vector2(packet->facingX, packet->facingY));
                remotePlayer->SetLightState(packet->isLightOn);
                remotePlayer->SetBushState(packet->isInBush);
            }
        }
    }
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
            Scene::Update();
            
            // ネットワーク処理
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
                // To do weapon sync later
                packet.currentWeaponIndex = 0;
                
                NetworkManager::GetInstance().SendPacket(&packet, sizeof(packet));
            }
            
            ProcessNetworkPackets();
        }
    }
    else if (state == GameState::PAUSED)
    {
        if (currEsc && !prevEsc)
        {
            state = GameState::PLAYING;
        }
        
        if (currUp && !prevUp) pauseMenuCursor--;
        if (currDown && !prevDown) pauseMenuCursor++;
        
        if (pauseMenuCursor < 0) pauseMenuCursor = 3;
        if (pauseMenuCursor > 3) pauseMenuCursor = 0;

        if (currEnter && !prevEnter)
        {
            if (pauseMenuCursor == 0) // Resume
            {
                state = GameState::PLAYING;
            }
            else if (pauseMenuCursor == 1) // Settings
            {
                state = GameState::SETTINGS;
                settingsMenuCursor = 0;
            }
            else if (pauseMenuCursor == 2) // Title
            {
                SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
            }
            else if (pauseMenuCursor == 3) // Exit
            {
                // To exit the game safely, we can post a quit message
                // However, since main loop checks ProcessMessage, we can just quit DxLib.
                // But a standard way is to return -1 or break.
                // We'll post a quit message to the OS.
                PostQuitMessage(0);
            }
        }
    }
    else if (state == GameState::SETTINGS)
    {
        if (currEsc && !prevEsc)
        {
            state = GameState::PAUSED;
        }

        if (currUp && !prevUp) settingsMenuCursor--;
        if (currDown && !prevDown) settingsMenuCursor++;
        
        if (settingsMenuCursor < 0) settingsMenuCursor = 4;
        if (settingsMenuCursor > 4) settingsMenuCursor = 0;

        if (currEnter && !prevEnter)
        {
            if (settingsMenuCursor == 0) // Toggle Aim Lock Mode
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (settingsMenuCursor == 1) // Toggle Debug View
            {
                isDebugView = !isDebugView;
            }
            else if (settingsMenuCursor == 2) // Change Theme
            {
                themeIdx = (themeIdx + 1) % 4;
                // Re-generate stage with new theme
                StageGenConfig config;
                config.theme = static_cast<ThemePattern>(themeIdx);
                config.variation = varIdx;
                ThemePattern currT = config.theme;
                int currV = config.variation;
                stage = StageGenerator::Generate(config, 12345, &currT, &currV);
                float cellW = 1920.0f / stage.GetWidth();
                float cellH = 1080.0f / stage.GetHeight();
                float cellSize = (cellW < cellH) ? cellW : cellH;
                if (player) {
                    Point2D startGrid = stage.GetPlayerStartPos();
                    player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                    player->SetStage(&stage, cellSize);
                }
            }
            else if (settingsMenuCursor == 3) // Change Variation
            {
                varIdx = (varIdx + 1) % 4;
                // Re-generate stage with new variation
                StageGenConfig config;
                config.theme = static_cast<ThemePattern>(themeIdx);
                config.variation = varIdx;
                ThemePattern currT = config.theme;
                int currV = config.variation;
                stage = StageGenerator::Generate(config, 12345, &currT, &currV);
                float cellW = 1920.0f / stage.GetWidth();
                float cellH = 1080.0f / stage.GetHeight();
                float cellSize = (cellW < cellH) ? cellW : cellH;
                if (player) {
                    Point2D startGrid = stage.GetPlayerStartPos();
                    player->SetPosition(Vector2((startGrid.x + 0.5f) * cellSize, (startGrid.y + 0.5f) * cellSize));
                    player->SetStage(&stage, cellSize);
                }
            }
            else if (settingsMenuCursor == 4) // Back
            {
                state = GameState::PAUSED;
            }
        }
    }

    prevEsc = currEsc;
    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
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

    if (player && player->IsActive() && !isDebugView)
    {
        // 描画エリアは 1920x1080
        float mapPixelWidth = stage.GetWidth() * cellSize;
        float mapPixelHeight = stage.GetHeight() * cellSize;
        float startDrawX = (1920 - mapPixelWidth) / 2.0f;
        float startDrawY = (1080 - mapPixelHeight) / 2.0f;

        // 【eitaの暗闇・ライト機能】
        player->RenderLightMask(0, 0, 1920, 1080, startDrawX, startDrawY);
        
        if (remotePlayer && remotePlayer->IsActive())
        {
            // Set draw blend mode manually to layer lights if needed, but RenderLightMask resets it anyway.
            // Note: In a real lighting system, we should combine the light masks, 
            // but calling it twice will draw over each other.
            // For simple implementation, drawing twice is okay or might look weird if intersecting.
            // Actually, we should probably combine them, but let's just call it.
            // However, RenderLightMask draws the *darkness*.
            // If both draw darkness, the screen will just be dark twice!
            // Wait, RenderLightMask draws black with Alpha where light is NOT present.
            // If we draw it twice, the areas with light for Player 2 will still be darkened by Player 1's mask!
            // We'll leave it as only the local player for now to prevent complete darkness, or we could just skip remote player's light mask for now as it requires a shader/render target to properly merge.
        }
    }
    
    // オーバーレイHUD (プレイ中も表示)
    DrawBox(12, 12, 400, 60, GetColor(15, 20, 30), TRUE);
    DrawBox(12, 12, 400, 60, GetColor(0, 180, 240), FALSE);
    DrawString(10, 10, "[ESC]キーでポーズ", GetColor(255, 255, 255));
    DrawString(10, 30, (std::string("Theme: ") + std::to_string(themeIdx + 1)).c_str(), GetColor(150, 150, 150));
    DrawString(10, 50, (std::string("Variation: ") + std::to_string(varIdx + 1)).c_str(), GetColor(150, 150, 150));

    if (state == GameState::PAUSED || state == GameState::SETTINGS)
    {
        // 半透明の黒背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, 0, 1920, 1080, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawString(1920 / 2 - 50, 200, "== ポーズ ==", GetColor(255, 255, 0));

        const int menuStartX = 1920 / 2 - 100;
        const int menuStartY = 400;
        const int menuSpacing = 60;
        
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
                "テーマ変更 (現在: " + std::to_string(themeIdx + 1) + ")", 
                "マップ変更 (現在: " + std::to_string(varIdx + 1) + ")", 
                "戻る" 
            };
            for (int i = 0; i < 5; i++)
            {
                unsigned int color = (i == settingsMenuCursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
                if (i == settingsMenuCursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
                DrawString(menuStartX, menuStartY + i * menuSpacing, items[i].c_str(), color);
            }
        }
        
        // 操作説明・ルールの描画（画面右側）
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
}
