# -*- coding: utf-8 -*-
import re

with open('TeamGame/Source/Scenes/GameScene.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Add includes
includes = '''#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "PacketTypes.h"
#include "GameSettings.h"
#include <algorithm>
#include <cmath>
#include <random>'''
content = re.sub(r'#include "GameScene\.h".*?<random>', includes, content, flags=re.DOTALL)

# Add ProcessNetworkPackets
process_network = '''
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
'''

content = content.replace('GameScene::~GameScene()\n{\n}', 'GameScene::~GameScene()\n{\n}\n' + process_network)

# Modify Init()
init_code = '''void GameScene::Init()
{
    Scene::Init();
    
    // StageManagerの初期化
    stageManager.Initialize(48, 27);
    isDebugView = false; // 最初から暗闇モード
    
    bool isMultiplayer = NetworkManager::GetInstance().IsConnected();
    bool isHostOrSingle = !isMultiplayer || NetworkManager::GetInstance().IsHost();
    
    // クライアント側は後でSTAGE_INITを受信して再生成するが、とりあえず初期状態を用意
    
    const Stage& stage = stageManager.GetCurrentStage();
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;
    
    Point2D startGrid = stage.GetPlayerStartPos();
    float startX = (startGrid.x + 0.5f) * cellSize;
    float startY = (startGrid.y + 0.5f) * cellSize;
    
    player = new Player(startX, startY);
    Stage* stagePtr = const_cast<Stage*>(&stageManager.GetCurrentStage());
    player->SetStage(stagePtr, cellSize);
    
    if (isMultiplayer)
    {
        remotePlayer = new Player(startX, startY);
        remotePlayer->SetStage(stagePtr, cellSize);
        remotePlayer->SetRemote(true);
    }
    else
    {
        remotePlayer = nullptr;
    }
    
    if (isHostOrSingle)
    {
        if (isMultiplayer)
        {
            PacketStageInit packet;
            packet.header.type = PacketType::STAGE_INIT;
            packet.seed = stageManager.GetCurrentSeed();
            packet.themeIdx = static_cast<int>(stageManager.GetCurrentTheme());
            packet.varIdx = stageManager.GetCurrentVariation();
            NetworkManager::GetInstance().SendPacket(&packet, sizeof(packet));
        }
        
        // 敵を水・壁・外枠を避けてプレイヤーから離れたランダム位置にスポーン
        SpawnEnemiesRandomly(5);
    }
}'''

content = re.sub(r'void GameScene::Init\(\).*?SpawnEnemiesRandomly\(5\);\n}', init_code, content, flags=re.DOTALL)

update_code = '''void GameScene::Update()
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
            Scene::Update(); // 自身の持つobjectManagerやcolliderManagerが実行される

            // Tabキー または F1キーで暗闇モード / デバッグ表示切替
            if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_TAB) ||
                InputManager::GetInstance().IsKeyPressed(KEY_INPUT_F1))
            {
                isDebugView = !isDebugView;
            }

            // Rキーでステージバリエーション切替
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

            // Tキーでテーマ切替
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

            // ネットワーク同期
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
            else if (settingsMenuCursor == 1) isDebugView = !isDebugView;
            else if (settingsMenuCursor == 2) { stageManager.NextTheme(); }
            else if (settingsMenuCursor == 3) { stageManager.NextVariation(); }
            else if (settingsMenuCursor == 4) state = GameState::PAUSED;
        }
    }

    prevEsc = currEsc;
    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
}'''

content = re.sub(r'void GameScene::Update\(\).*?ResultScene\>\(\)\);\n    }\n}', update_code, content, flags=re.DOTALL)

draw_code = '''void GameScene::Draw()
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

    // 1. プレイヤーの位置(playerWorldX, playerWorldY)を 1920x1080 画面中央 (960, 540) に配置するズームカメラ描画
    stage.DrawZoomCamera(playerWorldX, playerWorldY, zoomCellSize, worldCellSize, isDebugView, stageName.c_str(), -1);

    // 2. オブジェクト類 (プレイヤー・敵・弾丸) のカメラ相対描画
    Scene::Draw();

    // 3. プレイヤーを中心とするホラー暗闇スポットライトマスクの描画 (非デバッグ表示時)
    if (!isDebugView && player && player->IsActive())
    {
        player->RenderLightMask(0, 0, 1920, 1080, 0, 0);
    }
    
    // オーバーレイHUD
    DrawString(10, 10, "[ESC]キーでポーズ", GetColor(255, 255, 255));
    DrawString(10, 30, (std::string("Theme: ") + std::to_string((int)stageManager.GetCurrentTheme() + 1)).c_str(), GetColor(150, 150, 150));
    DrawString(10, 50, (std::string("Variation: ") + std::to_string(stageManager.GetCurrentVariation() + 1)).c_str(), GetColor(150, 150, 150));

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
'''

content = re.sub(r'void GameScene::Draw\(\).*?RenderLightMask\(0, 0, 1920, 1080, 0, 0\);\n    }\n}', draw_code, content, flags=re.DOTALL)

with open('TeamGame/Source/Scenes/GameScene.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
