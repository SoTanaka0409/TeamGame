#include "GameScene.h"
#include "Camera.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "CutinManager.h"
#include "GameBalanceManager.h"
#include "DebugManager.h"
#include "DxLib.h"
#include "Enemy.h"
#include "../Objects/Item.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "ClearScene.h"
#include "GameOverScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "GameSettings.h"
#include "../Weapons/Handgun.h"
#include "../Weapons/Shotgun.h"
#include "../Weapons/SniperRifle.h"
#include "../Core/PlayerSelection.h"
#include "../Core/CharacterFactory.h"
#include "../Managers/MatchManager.h"
#include "FontManager.h"
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
    int myTeam = player ? player->teamId : 0;
    for (auto obj : objectManager->GetObjects())
    {
        Character* c = dynamic_cast<Character*>(obj);
        if (c && c->IsActive() && !c->status.IsDead())
        {
            if (c->teamId != myTeam)
            {
                count++;
            }
        }
    }
    return count;
}

int GameScene::GetActiveAllyCount() const
{
    if (!objectManager) return 0;
    int count = 0;
    int myTeam = player ? player->teamId : 0;
    for (auto obj : objectManager->GetObjects())
    {
        Character* c = dynamic_cast<Character*>(obj);
        if (c && c->IsActive() && !c->status.IsDead())
        {
            if (c->teamId == myTeam && c != player)
            {
                count++;
            }
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
                obj->DestroyPermanently();
            }
        }
    }
    enemies.clear();
}

void GameScene::SpawnEnemiesRandomly(int count)
{
    ClearEnemies();
    totalEnemiesSpawned = count;

    const Stage& stage = stageManager.GetCurrentStage();
    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float cellSize = (cellW < cellH) ? cellW : cellH;

    // シングルプレイ: ボットの出現および武器配分処理
    CharacterFactory::SpawnMatchBots(stage, cellSize, player, objectManager, enemies);
}

void GameScene::Init()
{
    Scene::Init();
    gameTimer = 0.0f;
    isCleared = false;
    MatchManager::GetInstance().Reset();

    brightTimer = 0.0f;
    int brightMin = GameBalanceManager::GetInstance().GetInt("BrightEventMinIntervalFrames", 1200);
    int brightMax = GameBalanceManager::GetInstance().GetInt("BrightEventMaxIntervalFrames", 2100);
    int brightRange = (brightMax > brightMin) ? (brightMax - brightMin) : 900;
    nextBrightInterval = static_cast<float>(brightMin + (std::rand() % brightRange));
    sonarPingTimer = static_cast<float>(GameBalanceManager::GetInstance().GetInt("SonarPingIntervalFrames", 900));
    sonarNotificationTimer = 0.0f;
    
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
    
    Stage* stagePtr = const_cast<Stage*>(&stageManager.GetCurrentStage());
    player = CharacterFactory::CreatePlayer(startX, startY, stagePtr, cellSize, 0);
    if (objectManager) objectManager->AddObject(player);

    introTimer = static_cast<float>(GameBalanceManager::GetInstance().GetInt("IntroCameraDurationFrames", 300));
    SpawnEnemiesRandomly(GameBalanceManager::GetInstance().GetInt("InitialEnemyCount", 5));
}

void GameScene::Update()
{
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currLeft = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0);
    bool currRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);

    if (state == GameState::PLAYING)
    {
        if (currEsc && !prevEsc)
        {
            state = GameState::PAUSED;
            pauseMenuCursor = 0;
        }
        else
        {
            // スタイリッシュカットイン演出の更新
            CutinManager::GetInstance().Update();

            gameTimer += 0.016f;

            if (introTimer > 0.0f)
            {
                introTimer -= 1.0f;
            }
            else if (CutinManager::GetInstance().ShouldPauseGame())
            {
                // カットイン演出の強烈なインパクト期間中はゲーム世界をヒットストップ
            }
            else
            {
                Scene::Update();
            }

            // ランダムな時間帯に（15秒間）全エリアが明るくなる処理
            if (brightTimer > 0.0f)
            {
                brightTimer -= 1.0f;
            }
            else
            {
                nextBrightInterval -= 1.0f;
                if (nextBrightInterval <= 0.0f)
                {
                    brightTimer = static_cast<float>(GameBalanceManager::GetInstance().GetInt("BrightEventDurationFrames", 900));
                    int brightMin = GameBalanceManager::GetInstance().GetInt("BrightEventMinIntervalFrames", 1200);
                    int brightMax = GameBalanceManager::GetInstance().GetInt("BrightEventMaxIntervalFrames", 2100);
                    int brightRange = (brightMax > brightMin) ? (brightMax - brightMin) : 900;
                    nextBrightInterval = static_cast<float>(brightMin + (std::rand() % brightRange));
                }
            }

            // 15秒周期で双方が音波を発して互いの位置を探知する処理
            sonarPingTimer -= 1.0f;
            if (sonarNotificationTimer > 0.0f) sonarNotificationTimer -= 1.0f;

            if (sonarPingTimer <= 0.0f)
            {
                sonarPingTimer = static_cast<float>(GameBalanceManager::GetInstance().GetInt("SonarPingIntervalFrames", 900));
                sonarNotificationTimer = static_cast<float>(GameBalanceManager::GetInstance().GetInt("SonarNotificationFrames", 180));

                if (objectManager)
                {
                    for (auto obj : objectManager->GetObjects())
                    {
                        Character* ch = dynamic_cast<Character*>(obj);
                        if (ch && ch->IsActive())
                        {
                            Vector2 pos = ch->GetPosition();
                            unsigned int waveCol = (ch->teamId == 0) ? GetColor(0, 220, 255) : GetColor(255, 100, 50);
                            if (effectManager)
                            {
                                effectManager->AddSonarWaveEffect(pos.x, pos.y, waveCol);
                            }

                            // 音波により相手チームに位置を伝達
                            for (auto otherObj : objectManager->GetObjects())
                            {
                                Enemy* e = dynamic_cast<Enemy*>(otherObj);
                                if (e && e->IsActive() && e->teamId != ch->teamId && e->teamId != -1)
                                {
                                    e->OnHearGunshot(pos, 99999.0f, ch->teamId);
                                }
                            }
                        }
                    }
                }
                SoundManager::GetInstance().Play3D("gunshot", player ? player->GetPosition() : Vector2(960, 540), 1000.0f, 0.4f, 0);
            }

            // リスポーン処理およびキル数カウントをMatchManagerに移譲
            const Stage& stage = stageManager.GetCurrentStage();
            float cellW = 1920.0f / stage.GetWidth();
            float cellH = 1080.0f / stage.GetHeight();
            float cellSize = (cellW < cellH) ? cellW : cellH;

            MatchManager::GetInstance().UpdateRespawns(objectManager, stage, cellSize);
            MatchManager::GetInstance().UpdateKillLogs();

            // 勝敗判定 (MatchManagerの判定を使用)
            MatchResult result = MatchManager::GetInstance().CheckMatchResult();
            if (!isCleared && result != MatchResult::IN_PROGRESS)
            {
                isCleared = true;
                if (result == MatchResult::TEAM0_WIN)
                {
                    ClearStats stats;
                    stats.clearTimeSec = gameTimer;
                    stats.defeatedEnemies = MatchManager::GetInstance().GetTeam0Kills();
                    stats.totalEnemies = MatchManager::GetInstance().GetTargetKillCount();

                    int baseScore = 5000;
                    int timePenalty = static_cast<int>(gameTimer * 10.0f);
                    stats.rankScore = (baseScore > timePenalty) ? (baseScore - timePenalty) : 1000;

                    if (stats.rankScore >= 4500) stats.rankName = "S";
                    else if (stats.rankScore >= 3500) stats.rankName = "A";
                    else if (stats.rankScore >= 2500) stats.rankName = "B";
                    else stats.rankName = "C";

                    SceneManager::GetInstance().ChangeScene(std::make_shared<ClearScene>(stats));
                }
                else
                {
                    SceneManager::GetInstance().ChangeScene(std::make_shared<GameOverScene>());
                }
                return;
            }

            DebugManager::GetInstance().Update();

            if (DebugManager::GetInstance().IsDebugMode())
            {
                if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_L))
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
            }
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
            else if (pauseMenuCursor == 1)
            {
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
            }
            else if (pauseMenuCursor == 2)
            {
                state = GameState::SETTINGS;
                settingsMenuCursor = 0;
            }
            else if (pauseMenuCursor == 3)
            {
                SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
            }
        }
    }
    else if (state == GameState::SETTINGS)
    {
        if (currEsc && !prevEsc) state = GameState::PAUSED;
        if (currUp && !prevUp) settingsMenuCursor--;
        if (currDown && !prevDown) settingsMenuCursor++;
        if (settingsMenuCursor < 0) settingsMenuCursor = 1;
        if (settingsMenuCursor > 1) settingsMenuCursor = 0;

        if (currEnter && !prevEnter)
        {
            if (settingsMenuCursor == 0)
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (settingsMenuCursor == 1)
            {
                state = GameState::PAUSED;
            }
        }
    }

    prevEsc = currEsc;
    prevUp = currUp;
    prevDown = currDown;
    prevLeft = currLeft;
    prevRight = currRight;
    prevEnter = currEnter;
}

void GameScene::Draw()
{
    const Stage& stage = stageManager.GetCurrentStage();
    std::string stageName = stageManager.GetCurrentStageName();
    int activeEnemyCount = GetActiveEnemyCount();

    float cellW = 1920.0f / stage.GetWidth();
    float cellH = 1080.0f / stage.GetHeight();
    float worldCellSize = (cellW < cellH) ? cellW : cellH;

    bool isDebugView = DebugManager::GetInstance().IsDebugMode();
    float zoomCellSize = worldCellSize * 2.0f;

    if (introTimer > 0.0f)
    {
        if (introTimer > 180.0f)
        {
            zoomCellSize = worldCellSize;
        }
        else
        {
            float t = (180.0f - introTimer) / 180.0f;
            t = t * t * (3.0f - 2.0f * t);
            float introZoom = worldCellSize + (worldCellSize * 2.0f - worldCellSize) * t;
            zoomCellSize = introZoom;
        }
    }

    float playerWorldX = 0.0f, playerWorldY = 0.0f;
    if (player && player->IsActive())
    {
        Vector2 pos = player->GetPosition();
        playerWorldX = pos.x;
        playerWorldY = pos.y;
    }

    // 1画面（フルスクリーン）描画
    float enemySpawnY = 3 * worldCellSize;
    float playerSpawnY = (stage.GetHeight() - 4) * worldCellSize;
    
    if (introTimer > 0.0f)
    {
        float t = 1.0f - (introTimer / 300.0f);
        t = t * t * (3.0f - 2.0f * t);
        
        float midWorldX = (stage.GetWidth() / 2.0f) * worldCellSize;
        Camera::TargetWorldX = midWorldX;
        Camera::TargetWorldY = enemySpawnY + (playerSpawnY - enemySpawnY) * t;
    }
    else
    {
        Vector2 aimDir = (player && player->IsActive()) ? player->GetFacingDir() : Vector2(0.0f, 0.0f);
        float maxOffsetDist = 70.0f;
        if (player && player->GetCurrentWeapon())
        {
            if (player->GetCurrentWeapon()->GetName() == "SniperRifle")
            {
                maxOffsetDist = 200.0f;
            }
        }
        Vector2 targetOffset = aimDir * maxOffsetDist;

        m_cameraOffset.x += (targetOffset.x - m_cameraOffset.x) * 0.1f;
        m_cameraOffset.y += (targetOffset.y - m_cameraOffset.y) * 0.1f;

        Camera::TargetWorldX = playerWorldX + m_cameraOffset.x;
        Camera::TargetWorldY = playerWorldY + m_cameraOffset.y;
    }

    Camera::ScreenCenterX = 1920.0f / 2.0f;
    Camera::ScreenCenterY = 1080.0f / 2.0f;
    Camera::ZoomScale = zoomCellSize / worldCellSize;

    stage.DrawZoomCamera(playerWorldX, playerWorldY, zoomCellSize, worldCellSize, isDebugView, stageName.c_str(), -1);
    Scene::Draw();
    stage.DrawForegroundGrass(playerWorldX, playerWorldY, zoomCellSize, worldCellSize);

    // 薄明イベント時の環境光比率計算 (通常0.0f -> 薄明時1.0f)
    float brightRatio = 0.0f;
    if (brightTimer > 0.0f)
    {
        if (brightTimer > 840.0f) brightRatio = (900.0f - brightTimer) / 60.0f;
        else if (brightTimer < 60.0f) brightRatio = brightTimer / 60.0f;
        else brightRatio = 1.0f;
    }

    if (!DebugManager::GetInstance().IsDebugMode() && player && player->IsActive())
    {
        if (introTimer <= 0.0f) 
        {
            player->RenderLightMask(0, 0, 1920, 1080, 0, 0, brightRatio);
        }
    }

    // HighRiskスキル: 敵の位置・距離(m)・矢印の描画
    if (player && player->IsActive())
    {
        player->RenderHighRiskEnemyIndicators();
    }

    // 体力低下時の血しぶきデンジャーエフェクト
    if (GameSettings::GetInstance().isBloodSplatterEnabled && player && player->IsActive())
    {
        player->RenderBloodSplatterOverlay(1920, 1080);
    }
    
    DrawString(15, 15, "[ESC]キーでポーズ", GetColor(255, 255, 255));
    DrawString(15, 45, (std::string("Theme: ") + std::to_string((int)stageManager.GetCurrentTheme() + 1)).c_str(), GetColor(200, 200, 200));
    DrawString(15, 70, (std::string("Variation: ") + std::to_string(stageManager.GetCurrentVariation() + 1)).c_str(), GetColor(200, 200, 200));

    // 薄明時間帯（月光照射）のUIバナー表示
    if (brightTimer > 0.0f)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(1920 / 2 - 250, 65, 1920 / 2 + 250, 105, GetColor(250, 200, 50), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(1920 / 2 - 250, 65, 1920 / 2 + 250, 105, GetColor(255, 255, 255), FALSE);
        DrawFormatString(1920 / 2 - 230, 75, GetColor(0, 0, 0), "【薄明・月光時間帯】 月明かり照射中 (残り %.1f秒)", brightTimer / 60.0f);
    }

    // 15秒周期の相互音波ピン探知通知バナー表示
    if (sonarNotificationTimer > 0.0f)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
        DrawBox(1920 / 2 - 260, 115, 1920 / 2 + 260, 155, GetColor(0, 150, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(1920 / 2 - 260, 115, 1920 / 2 + 260, 155, GetColor(255, 255, 255), FALSE);
        DrawString(1920 / 2 - 240, 125, "【音波探知発信】 全員の互いの位置をソナー探知！", GetColor(255, 255, 255));
    }

    // 画面左下にプレイヤーのHP・武器UIを表示
    if (player && player->IsActive()) {
        player->DrawUI(25, 850);
    }

    if (state == GameState::PAUSED || state == GameState::SETTINGS)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, 0, 1920, 1080, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawString(1920 / 2 - 50, 200, "== ポーズ ==", GetColor(255, 255, 0));

        const int menuStartX = 1920 / 2 - 100;
        const int menuStartY = 400;
        const int menuSpacing = 55;

        if (state == GameState::PLAYING && introTimer > 0.0f)
        {
            SetFontSize(80);
            if (introTimer > 60.0f) {
                DrawString(1920/2 - 150, 1080/2 - 100, "READY...", GetColor(255, 200, 50));
            } else {
                DrawString(1920/2 - 180, 1080/2 - 100, "BRAWL!", GetColor(255, 50, 50));
            }
            SetFontSize(32);
        }

        if (state == GameState::PAUSED)
        {
            const char* items[] = { "Resume", "Restart", "Settings", "Exit to Title" };
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
                std::string("Aim Lock Mode (E key) : ") + (GameSettings::GetInstance().isAimLockHoldMode ? "Hold" : "Toggle"),
                "Back"
            };
            for (int i = 0; i < 2; i++)
            {
                unsigned int color = (i == settingsMenuCursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
                if (i == settingsMenuCursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
                DrawString(menuStartX, menuStartY + i * menuSpacing, items[i].c_str(), color);
            }
        }
    }

    // 画面上部中央: チームキル数バー (Brawl Stars風HUD)
    int maxKills = MatchManager::GetInstance().GetTargetKillCount();
    int barWidth = 460;
    int barHeight = 24;
    int startX = 1920 / 2 - barWidth / 2;
    int startY = 15;

    // バトルロワイヤル状態（Botが錯乱中）かどうかの判定
    bool isBattleRoyaleActive = false;
    int panicRemainingFrames = 0;
    if (objectManager) {
        for (auto obj : objectManager->GetObjects()) {
            Character* ch = dynamic_cast<Character*>(obj);
            if (ch && ch != player && ch->IsActive() && ch->IsPanic()) {
                isBattleRoyaleActive = true;
                int frames = ch->GetEffectRemainingFrames(SkillMinorTag::PanicShoot);
                if (frames > panicRemainingFrames) panicRemainingFrames = frames;
            }
        }
    }

    if (isBattleRoyaleActive)
    {
        int brW = 540;
        int brH = 58;
        int brX = (1920 - brW) / 2;
        int brY = 16;

        float timeSec = GetNowCount() / 1000.0f;
        int pulseAlpha = static_cast<int>(180 + std::sin(timeSec * 12.0f) * 60);

        // 背景ボックス（黒＆マゼンタ）
        DrawBox(brX, brY, brX + brW, brY + brH, GetColor(15, 10, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
        DrawBox(brX + 2, brY + 2, brX + brW - 2, brY + brH - 2, GetColor(220, 20, 120), FALSE);
        DrawBox(brX + 4, brY + 4, brX + brW - 4, brY + brH - 4, GetColor(255, 50, 50), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // バトロワ警告テキスト
        FontManager::GetInstance().DrawStringCenter(1920 / 2, brY + 6, "!! BATTLE ROYALE : ALL-OUT WAR !!", GetColor(255, 230, 40), 26, 9);

        char brSubText[64];
        snprintf(brSubText, sizeof(brSubText), "全員孤立・敵対中！ 残り時間: %.1f秒", panicRemainingFrames / 60.0f);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, brY + 34, brSubText, GetColor(255, 255, 255), 18, 6);
    }
    else
    {
        // 背景バー
        DrawBox(startX, startY, startX + barWidth, startY + barHeight, GetColor(40, 40, 40), TRUE);
        DrawBox(startX, startY + 32, startX + barWidth, startY + 32 + barHeight, GetColor(40, 40, 40), TRUE);

        int team0Kills = MatchManager::GetInstance().GetTeam0Kills();
        int team1Kills = MatchManager::GetInstance().GetTeam1Kills();

        // 自チーム（味方/プレイヤー）のキル数バー (青)
        int allyBarW = (int)((float)team0Kills / maxKills * barWidth);
        DrawBox(startX, startY, startX + allyBarW, startY + barHeight, GetColor(50, 150, 255), TRUE);
        FontManager::GetInstance().DrawFormatString(startX - 230, startY + 1, GetColor(255, 255, 255), 20, "味方チーム (KILLS): %d/%d", team0Kills, maxKills);

        // 敵チームのキル数バー (赤)
        int enemyBarW = (int)((float)team1Kills / maxKills * barWidth);
        DrawBox(startX, startY + 32, startX + enemyBarW, startY + 32 + barHeight, GetColor(255, 60, 60), TRUE);
        FontManager::GetInstance().DrawFormatString(startX - 230, startY + 33, GetColor(255, 255, 255), 20, "敵チーム (KILLS): %d/%d", team1Kills, maxKills);
    }

    int activeAllyCount = GetActiveAllyCount();
    DebugManager::GetInstance().DrawDebugOverlay(stageName, playerWorldX, playerWorldY, activeEnemyCount, activeAllyCount);

    // キルログ・デスログ通知バナーの描画
    MatchManager::GetInstance().DrawKillNotifications();

    // スキル発動時のペルソナ5風スタイリッシュカットイン描画（最前面オーバーレイ）
    CutinManager::GetInstance().Draw();
}
