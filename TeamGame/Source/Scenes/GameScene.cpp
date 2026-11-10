#include "../UI/ControlsGuide.h"
#include "../UI/HudStyle.h"
#include "GameScene.h"
#include "Camera.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "SoundVisualizer.h"
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
    SoundVisualizer::GetInstance().Clear();

    brightTimer = 0.0f;
    int brightMin = GameBalanceManager::GetInstance().GetInt("BrightEventMinIntervalFrames", 1200);
    int brightMax = GameBalanceManager::GetInstance().GetInt("BrightEventMaxIntervalFrames", 2100);
    int brightRange = (brightMax > brightMin) ? (brightMax - brightMin) : 900;
    // 初回ソナー音波は開始3秒(180フレーム)後、以降は900フレーム(15秒)周期で発生
    sonarPingTimer = 180.0f;
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

    // 歩行足音SE (草: grass_walk, 土: dirt_walk) のロード
    SoundManager::GetInstance().Load("grass_walk", "SE/足音/grass_walk.mp3");
    SoundManager::GetInstance().Load("dirt_walk", "SE/足音/dirt_walk.mp3");
    // 武器発射音・リロード音のロード
    SoundManager::GetInstance().Load("handgun_shot", "SE/銃/gun_handgun.mp3");
    SoundManager::GetInstance().Load("smg_shot", "SE/銃/gun_smg.mp3");
    SoundManager::GetInstance().Load("shotgun_shot", "SE/銃/gun_shotgun.mp3");
    SoundManager::GetInstance().Load("sniper_shot", "SE/銃/gun_sniper.mp3");
    SoundManager::GetInstance().Load("gunshot", "SE/銃/gun_assault.mp3");
    SoundManager::GetInstance().Load("reload", "SE/銃/gun_reload.mp3");
    SoundManager::GetInstance().Load("se_tap", "SE/システム/se_tap.mp3");
    SoundManager::GetInstance().Load("clear_bgm", "BGM/bgm_clear.mp3");
    SoundManager::GetInstance().Load("defeat_bgm", "BGM/bgm_defeat.mp3");
    SoundManager::GetInstance().Load("skill_activate", "SE/システム/se_skill.mp3");

    // 戦闘BGMの再生（音量をほんの少し下げて戦闘.mp3をループ再生: 0.60f）
    SoundManager::GetInstance().PlayBGM("battle_bgm", 0.60f);
}

void GameScene::Update()
{
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currLeft = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0);
    bool currRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currClick = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;

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
                // ホスト側はイントロ中にステージ初期化情報を定期送信して同期を確実にする
                
            }
            else if (CutinManager::GetInstance().ShouldPauseGame())
            {
                // カットイン演出の強烈なインパクト期間中はゲーム世界をヒットストップ
            }
            else
            {
                Scene::Update();
            }

            // 3D空間オーディオのリスナー（プレイヤー）位置・向きの更新およびサウンド解放
            if (player && player->IsActive())
            {
                SoundManager::GetInstance().SetListener(player->GetPosition(), player->GetFacingDir());
            }
            SoundManager::GetInstance().Update();
            SoundVisualizer::GetInstance().Update();

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

                            // 敵キャラクター（自チーム以外）がソナーを発した場合、ライトの範囲内にいない時のみ視覚インジケーターに登録
                            if (ch->teamId != 0)
                            {
                                if (!player || !player->IsWorldPosInLight(pos))
                                {
                                    SoundVisualizer::GetInstance().AddSound(SoundVisualType::SonarPing, pos, 2500.0f);
                                }
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
    else if (state == GameState::PAUSED)
    {
        if (currEsc && !prevEsc) state = GameState::PLAYING;
        if (currUp && !prevUp) pauseMenuCursor--;
        if (currDown && !prevDown) pauseMenuCursor++;
        if (pauseMenuCursor < 0) pauseMenuCursor = 3;
        if (pauseMenuCursor > 3) pauseMenuCursor = 0;

        int mouseX = 0, mouseY = 0;
        GetMousePoint(&mouseX, &mouseY);
        const int panelW = 680;
        const int panelH = 580;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;
        const int itemW = 580;
        const int itemH = 76;
        const int itemSpacing = 88;
        const int itemStartX = panelX + (panelW - itemW) / 2;
        const int itemStartY = panelY + 115;

        bool mouseClickedItem = false;
        for (int i = 0; i < 4; i++)
        {
            int iy = itemStartY + i * itemSpacing;
            if (mouseX >= itemStartX && mouseX <= itemStartX + itemW &&
                mouseY >= iy && mouseY <= iy + itemH)
            {
                pauseMenuCursor = i;
                if (currClick && !prevClick)
                {
                    mouseClickedItem = true;
                }
            }
        }

        if ((currEnter && !prevEnter) || mouseClickedItem)
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
        if (settingsMenuCursor < 0) settingsMenuCursor = 2;
        if (settingsMenuCursor > 2) settingsMenuCursor = 0;

        int mouseX = 0, mouseY = 0;
        GetMousePoint(&mouseX, &mouseY);
        const int panelW = 680;
        const int panelH = 500;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;
        const int itemW = 580;
        const int itemH = 76;
        const int itemSpacing = 95;
        const int itemStartX = panelX + (panelW - itemW) / 2;
        const int itemStartY = panelY + 130;

        bool mouseClickedItem = false;
        for (int i = 0; i < 3; i++)
        {
            int iy = itemStartY + i * itemSpacing;
            if (mouseX >= itemStartX && mouseX <= itemStartX + itemW &&
                mouseY >= iy && mouseY <= iy + itemH)
            {
                settingsMenuCursor = i;
                if (currClick && !prevClick)
                {
                    mouseClickedItem = true;
                }
            }
        }

        if ((currEnter && !prevEnter) || mouseClickedItem)
        {
            if (settingsMenuCursor == 0)
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (settingsMenuCursor == 1)
            {
                state = GameState::CONTROLS;
            }
            else if (settingsMenuCursor == 2)
            {
                state = GameState::PAUSED;
            }
        }
    }
    else if (state == GameState::CONTROLS)
    {
        if ((currEnter && !prevEnter) || (currEsc && !prevEsc) || (currClick && !prevClick))
        {
            state = GameState::SETTINGS;
        }
    }

    bool curHelp = (CheckHitKey(KEY_INPUT_0) != 0 || CheckHitKey(KEY_INPUT_NUMPAD0) != 0 || CheckHitKey(KEY_INPUT_H) != 0);
    prevHelpKey = curHelp;
    prevEsc = currEsc;
    prevUp = currUp;
    prevDown = currDown;
    prevLeft = currLeft;
    prevRight = currRight;
    prevEnter = currEnter;
    prevClick = currClick;
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
    // 画面に見える範囲を2/3に拡大（カメラズーム倍率を1.5倍にしてキャラクターとマップを大きく表示）
    float zoomCellSize = worldCellSize * 3.0f;

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
            float introZoom = worldCellSize + (worldCellSize * 3.0f - worldCellSize) * t;
            zoomCellSize = introZoom;
        }
    }

    float playerWorldX = 0.0f, playerWorldY = 0.0f;
    if (player)
    {
        Vector2 pos = player->GetPosition();
        playerWorldX = pos.x;
        playerWorldY = pos.y;
    }

    // 1画面（フルスクリーン）描画
    float enemySpawnY = 3 * worldCellSize;
    float playerSpawnY = (stage.GetHeight() - 4) * worldCellSize;
    
    Camera::ScreenCenterX = 1920.0f / 2.0f;
    Camera::ScreenCenterY = 1080.0f / 2.0f;
    Camera::ZoomScale = zoomCellSize / worldCellSize;

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
        Vector2 aimDir = (player && player->IsActive() && !player->IsDying()) ? player->GetFacingDir() : Vector2(0.0f, 0.0f);
        float maxOffsetDist = 70.0f;
        if (player && player->GetCurrentWeapon())
        {
            if (player->GetCurrentWeapon()->GetName() == "SniperRifle")
            {
                maxOffsetDist = 200.0f;
            }
        }
        Vector2 targetOffset = aimDir * maxOffsetDist;

        // 元の広大なエイム先読みを基本としつつ、画面端から「ほんの1マス分（1タイル）」だけ内側に収める
        float currentZoom = (Camera::ZoomScale > 0.001f) ? Camera::ZoomScale : 1.0f;
        float margin = worldCellSize; // 画面端からちょうど1マス分の安全マージン
        float maxOffsetX = (Camera::ScreenCenterX / currentZoom) - margin;
        float maxOffsetY = (Camera::ScreenCenterY / currentZoom) - margin;

        if (targetOffset.x > maxOffsetX) targetOffset.x = maxOffsetX;
        if (targetOffset.x < -maxOffsetX) targetOffset.x = -maxOffsetX;
        if (targetOffset.y > maxOffsetY) targetOffset.y = maxOffsetY;
        if (targetOffset.y < -maxOffsetY) targetOffset.y = -maxOffsetY;

        m_cameraOffset.x += (targetOffset.x - m_cameraOffset.x) * 0.1f;
        m_cameraOffset.y += (targetOffset.y - m_cameraOffset.y) * 0.1f;

        if (m_cameraOffset.x > maxOffsetX) m_cameraOffset.x = maxOffsetX;
        if (m_cameraOffset.x < -maxOffsetX) m_cameraOffset.x = -maxOffsetX;
        if (m_cameraOffset.y > maxOffsetY) m_cameraOffset.y = maxOffsetY;
        if (m_cameraOffset.y < -maxOffsetY) m_cameraOffset.y = -maxOffsetY;

        Camera::TargetWorldX = playerWorldX + m_cameraOffset.x;
        Camera::TargetWorldY = playerWorldY + m_cameraOffset.y;
    }

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

    // 音無し環境でも敵の方向が分かるビジュアルサウンドインジケーター（荒野行動風）
    if (player && player->IsActive())
    {
        float pScreenX = Camera::WorldToScreenX(playerWorldX);
        float pScreenY = Camera::WorldToScreenY(playerWorldY);
        SoundVisualizer::GetInstance().Draw(player, pScreenX, pScreenY);
    }

    // 体力低下時の血しぶきデンジャーエフェクト
    if (GameSettings::GetInstance().isBloodSplatterEnabled && player && player->IsActive())
    {
        player->RenderBloodSplatterOverlay(1920, 1080);
    }
    
    // 左上: [ESC] MENU のみ（背景プレートなし、控えめなサブテキスト色）
    FontManager::GetInstance().DrawString(24, 18, "[ESC] MENU", HudStyle::ColSubText(), 15);

    // デバッグ情報 (Theme / Variation) はデバッグモード時のみ表示
    if (DebugManager::GetInstance().IsDebugMode())
    {
        FontManager::GetInstance().DrawString(24, 40, (std::string("Theme: ") + std::to_string((int)stageManager.GetCurrentTheme() + 1)).c_str(), HudStyle::ColSubText(), 14);
        FontManager::GetInstance().DrawString(24, 58, (std::string("Variation: ") + std::to_string(stageManager.GetCurrentVariation() + 1)).c_str(), HudStyle::ColSubText(), 14);
    }

    // 画面上部中央: 統合キル数パネル (プロフェッショナル HUD)
    int maxKills = MatchManager::GetInstance().GetTargetKillCount();
    int topPanelW = 680;
    int topPanelH = 32;
    int topPanelX = (1920 - topPanelW) / 2;
    int topPanelY = 18;

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

        HudStyle::DrawPanel(brX, brY, brW, brH, true, HudStyle::ColEnemy(), pulseAlpha);

        // バトロワ警告テキスト
        FontManager::GetInstance().DrawStringCenter(1920 / 2, brY + 6, "!! BATTLE ROYALE : ALL-OUT WAR !!", HudStyle::ColAmber(), 26, 9);

        char brSubText[64];
        snprintf(brSubText, sizeof(brSubText), "全員孤立・敵対中！ 残り時間: %.1f秒", panicRemainingFrames / 60.0f);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, brY + 34, brSubText, HudStyle::ColText(), 18, 6);
    }
    else
    {
        // 1本の一体型パネル
        HudStyle::DrawPanel(topPanelX, topPanelY, topPanelW, topPanelH, true);

        int team0Kills = MatchManager::GetInstance().GetTeam0Kills();
        int team1Kills = MatchManager::GetInstance().GetTeam1Kills();

        // 左: 味方キル数 (鋼色)
        char allyText[64];
        snprintf(allyText, sizeof(allyText), "ALLY  %d / %d", team0Kills, maxKills);
        FontManager::GetInstance().DrawString(topPanelX + 16, topPanelY + 8, allyText, HudStyle::ColAlly(), 15);

        // 右: 敵キル数 (赤)
        char enemyText[64];
        snprintf(enemyText, sizeof(enemyText), "%d / %d  ENEMY", team1Kills, maxKills);
        int enemyW = FontManager::GetInstance().GetStringWidth(enemyText, 15);
        FontManager::GetInstance().DrawString(topPanelX + topPanelW - 16 - enemyW, topPanelY + 8, enemyText, HudStyle::ColEnemy(), 15);

        // 中央: 対向ゲージ (味方は中央から左へ、敵は中央から右へ)
        int centerX = 1920 / 2;
        int halfGaugeW = 120;
        int gaugeH = 6;
        int gaugeY = topPanelY + (topPanelH - gaugeH) / 2;

        float allyRatio = (maxKills > 0) ? static_cast<float>(team0Kills) / maxKills : 0.0f;
        float enemyRatio = (maxKills > 0) ? static_cast<float>(team1Kills) / maxKills : 0.0f;

        // 味方ゲージ (中央から左へ伸長)
        HudStyle::DrawSplitGauge(centerX - 4 - halfGaugeW, gaugeY, halfGaugeW, gaugeH, allyRatio, HudStyle::ColAlly(), true);
        // 敵ゲージ (中央から右へ伸長)
        HudStyle::DrawSplitGauge(centerX + 4, gaugeY, halfGaugeW, gaugeH, enemyRatio, HudStyle::ColEnemy(), false);

        // 中央セパレータティック
        DrawBox(centerX - 1, topPanelY + 8, centerX + 1, topPanelY + topPanelH - 8, HudStyle::ColBorder(), TRUE);
    }

    // 上部中央: 通知バナーの縦積みスタック（重なりを防止）
    int bannerStackY = (isBattleRoyaleActive ? 80 : topPanelY + topPanelH + 8);

    // 薄明時間帯（MOONLIGHT）バナー
    if (brightTimer > 0.0f)
    {
        char moonBuf[64];
        snprintf(moonBuf, sizeof(moonBuf), "MOONLIGHT ACTIVE  %.1fs", brightTimer / 60.0f);
        HudStyle::DrawBanner(1920 / 2, bannerStackY, 320, 26, moonBuf, HudStyle::ColAmber(), 14);
        bannerStackY += 26 + 6;
    }

    // ソナーピン（SONAR PING）探知バナー
    if (sonarNotificationTimer > 0.0f)
    {
        HudStyle::DrawBanner(1920 / 2, bannerStackY, 340, 26, "SONAR PING DETECTED", HudStyle::ColAlly(), 14);
        bannerStackY += 26 + 6;
    }

    // 画面左下にプレイヤーのHP・武器UIを表示
    if (player && player->IsActive()) {
        player->DrawUI(24, 904);
    }
    

    if (state == GameState::PLAYING && introTimer > 0.0f)
    {
        SetFontSize(80);
        if (introTimer > 60.0f) {
            DrawString(1920 / 2 - 150, 1080 / 2 - 100, "READY...", GetColor(255, 200, 50));
        } else {
            DrawString(1920 / 2 - 180, 1080 / 2 - 100, "BRAWL!", GetColor(255, 50, 50));
        }
        SetFontSize(32);
    }

    int activeAllyCount = GetActiveAllyCount();
    DebugManager::GetInstance().DrawDebugOverlay(stageName, playerWorldX, playerWorldY, activeEnemyCount, activeAllyCount);

    // キルログ・デスログ通知バナーの描画
    MatchManager::GetInstance().DrawKillNotifications();

    // スキル発動時のペルソナ5風スタイリッシュカットイン描画（最前面オーバーレイ）
    CutinManager::GetInstance().Draw();

    // ==========================================
    // 最前面オーバーレイメニュー (ポーズ / 設定 / 操作説明)
    // ==========================================

    if (state == GameState::PAUSED)
    {
        // 1. 全面暗転オーバーレイ (背景を適度に暗くして集中)
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
        DrawBox(0, 0, 1920, 1080, GetColor(5, 10, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインパネル枠
        const int panelW = 680;
        const int panelH = 580;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;

        // パネル背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(16, 22, 34), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 多重ネオン外枠
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 180, 255), FALSE);
        DrawBox(panelX + 2, panelY + 2, panelX + panelW - 2, panelY + panelH - 2, GetColor(0, 120, 180), FALSE);
        DrawBox(panelX + 4, panelY + 4, panelX + panelW - 4, panelY + panelH - 4, GetColor(30, 50, 80), FALSE);

        // ヘッダータイトル
        int headerY = panelY + 22;
        FontManager::GetInstance().DrawStringCenter(1920 / 2, headerY, "PAUSE MENU", GetColor(255, 220, 0), 30);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, headerY + 36, "- 一時停止 / メニュー選択 -", GetColor(160, 200, 240), 16);
        DrawLine(panelX + 40, headerY + 64, panelX + panelW - 40, headerY + 64, GetColor(0, 140, 200));

        // 3. 各項目の枠（カード・ボタン）
        const int itemW = 580;
        const int itemH = 76;
        const int itemSpacing = 88;
        const int itemStartX = panelX + (panelW - itemW) / 2;
        const int itemStartY = panelY + 115;

        struct MenuItemInfo {
            const char* titleJa;
            const char* titleEn;
            const char* desc;
            const char* badge;
            bool isSpecial;
        };

        const MenuItemInfo items[4] = {
            { "ゲーム再開", "RESUME GAME", "戦闘画面に戻ってプレイを続行します", "▶", false },
            { "やり直し", "RESTART", "現在のステージを最初からリスタートします", "↺", false },
            { "各種設定", "SETTINGS", "エイム固定モード等の設定を変更します", "⚙", false },
            { "タイトルへ", "EXIT TO TITLE", "タイトル画面へ戻ります", "✕", false }
        };

        for (int i = 0; i < 4; i++)
        {
            int iy = itemStartY + i * itemSpacing;
            bool isSelected = (i == pauseMenuCursor);

            // 枠の背景
            if (isSelected)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 235);
                unsigned int bgCol = items[i].isSpecial ? GetColor(45, 48, 25) : GetColor(25, 45, 75);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, bgCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // 発光枠線
                unsigned int borderCol = items[i].isSpecial ? GetColor(255, 215, 0) : GetColor(0, 220, 255);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, borderCol, FALSE);
                DrawBox(itemStartX + 1, iy + 1, itemStartX + itemW - 1, iy + itemH - 1, borderCol, FALSE);

                // 左端のアクティブバー
                DrawBox(itemStartX, iy, itemStartX + 8, iy + itemH, borderCol, TRUE);
            }
            else
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, GetColor(12, 18, 28), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // 通常枠線（操作説明枠は非選択時でもゴールド系の視認しやすい枠線）
                unsigned int borderCol = items[i].isSpecial ? GetColor(160, 130, 40) : GetColor(45, 65, 95);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, borderCol, FALSE);
            }

            // バッジ枠（左側アイコン）
            int badgeX = itemStartX + 18;
            int badgeY = iy + 14;
            int badgeSize = 48;
            unsigned int badgeBorder = isSelected ? (items[i].isSpecial ? GetColor(255, 215, 0) : GetColor(0, 220, 255)) : GetColor(50, 70, 95);
            unsigned int badgeBg = isSelected ? GetColor(35, 55, 85) : GetColor(20, 28, 42);
            DrawBox(badgeX, badgeY, badgeX + badgeSize, badgeY + badgeSize, badgeBg, TRUE);
            DrawBox(badgeX, badgeY, badgeX + badgeSize, badgeY + badgeSize, badgeBorder, FALSE);
            unsigned int badgeTextCol = isSelected ? GetColor(255, 255, 255) : GetColor(160, 180, 200);
            FontManager::GetInstance().DrawStringCenter(badgeX + badgeSize / 2, badgeY + 10, items[i].badge, badgeTextCol, 22);

            // テキスト描画
            int textX = badgeX + badgeSize + 18;
            unsigned int titleCol = isSelected ? (items[i].isSpecial ? GetColor(255, 235, 80) : GetColor(255, 240, 100)) : GetColor(220, 225, 235);
            FontManager::GetInstance().DrawString(textX, iy + 12, items[i].titleJa, titleCol, 22);

            // 英語サブタイトル
            unsigned int enCol = isSelected ? GetColor(140, 200, 255) : GetColor(120, 135, 150);
            FontManager::GetInstance().DrawString(textX + 130, iy + 16, items[i].titleEn, enCol, 15);

            // 説明文
            unsigned int descCol = isSelected ? GetColor(200, 220, 240) : GetColor(130, 145, 160);
            FontManager::GetInstance().DrawString(textX, iy + 44, items[i].desc, descCol, 15);

            // 選択中のカーソル矢印
            if (isSelected)
            {
                FontManager::GetInstance().DrawString(itemStartX + itemW - 35, iy + 24, "▶", items[i].isSpecial ? GetColor(255, 215, 0) : GetColor(0, 220, 255), 22);
            }
        }

        // フッター案内
        int footerY = panelY + panelH - 42;
        FontManager::GetInstance().DrawStringCenter(1920 / 2, footerY, "[ ↑ / ↓ / マウス ] 選択    [ Enter / クリック ] 決定    [ ESC ] 再開", GetColor(160, 190, 220), 16);
    }
    else if (state == GameState::SETTINGS)
    {
        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
        DrawBox(0, 0, 1920, 1080, GetColor(5, 10, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインパネル枠
        const int panelW = 680;
        const int panelH = 500;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(16, 22, 34), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 180, 255), FALSE);
        DrawBox(panelX + 2, panelY + 2, panelX + panelW - 2, panelY + panelH - 2, GetColor(0, 120, 180), FALSE);
        DrawBox(panelX + 4, panelY + 4, panelX + panelW - 4, panelY + panelH - 4, GetColor(30, 50, 80), FALSE);

        int headerY = panelY + 22;
        FontManager::GetInstance().DrawStringCenter(1920 / 2, headerY, "SETTINGS", GetColor(255, 220, 0), 30);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, headerY + 36, "- 各種設定 -", GetColor(160, 200, 240), 16);
        DrawLine(panelX + 40, headerY + 64, panelX + panelW - 40, headerY + 64, GetColor(0, 140, 200));

        const int itemW = 580;
        const int itemH = 76;
        const int itemSpacing = 95;
        const int itemStartX = panelX + (panelW - itemW) / 2;
        const int itemStartY = panelY + 120;

        std::string modeText = std::string("エイム固定 (Eキー): ") + (GameSettings::GetInstance().isAimLockHoldMode ? "長押し (Hold)" : "切り替え (Toggle)");
        std::string modeDesc = GameSettings::GetInstance().isAimLockHoldMode ? "Eキーを押している間だけエイムを固定します" : "Eキーを押すごとにエイム固定のON/OFFを切り替えます";

        struct SettingsItemInfo {
            std::string title;
            std::string desc;
            const char* badge;
            bool isSpecial;
        };

        SettingsItemInfo sItems[3] = {
            { modeText, modeDesc, "⚙", false },
            { "操作説明 (CONTROLS GUIDE)", "キーボード・マウス操作＆ルールのヘルプを表示", "？", true },
            { "戻る (BACK)", "ポーズメニューへ戻ります", "←", false }
        };

        for (int i = 0; i < 3; i++)
        {
            int iy = itemStartY + i * itemSpacing;
            bool isSelected = (i == settingsMenuCursor);

            if (isSelected)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 235);
                unsigned int bgCol = sItems[i].isSpecial ? GetColor(45, 48, 25) : GetColor(25, 45, 75);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, bgCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                unsigned int borderCol = sItems[i].isSpecial ? GetColor(255, 215, 0) : GetColor(0, 220, 255);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, borderCol, FALSE);
                DrawBox(itemStartX + 1, iy + 1, itemStartX + itemW - 1, iy + itemH - 1, borderCol, FALSE);
                DrawBox(itemStartX, iy, itemStartX + 8, iy + itemH, borderCol, TRUE);
            }
            else
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, GetColor(12, 18, 28), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                unsigned int borderCol = sItems[i].isSpecial ? GetColor(160, 130, 40) : GetColor(45, 65, 95);
                DrawBox(itemStartX, iy, itemStartX + itemW, iy + itemH, borderCol, FALSE);
            }

            int badgeX = itemStartX + 18;
            int badgeY = iy + 14;
            int badgeSize = 48;
            unsigned int badgeBorder = isSelected ? (sItems[i].isSpecial ? GetColor(255, 215, 0) : GetColor(0, 220, 255)) : GetColor(50, 70, 95);
            unsigned int badgeBg = isSelected ? GetColor(35, 55, 85) : GetColor(20, 28, 42);
            DrawBox(badgeX, badgeY, badgeX + badgeSize, badgeY + badgeSize, badgeBg, TRUE);
            DrawBox(badgeX, badgeY, badgeX + badgeSize, badgeY + badgeSize, badgeBorder, FALSE);
            unsigned int badgeTextCol = isSelected ? GetColor(255, 255, 255) : GetColor(160, 180, 200);
            FontManager::GetInstance().DrawStringCenter(badgeX + badgeSize / 2, badgeY + 10, sItems[i].badge, badgeTextCol, 22);

            int textX = badgeX + badgeSize + 18;
            unsigned int titleCol = isSelected ? (sItems[i].isSpecial ? GetColor(255, 235, 80) : GetColor(255, 240, 100)) : GetColor(220, 225, 235);
            FontManager::GetInstance().DrawString(textX, iy + 12, sItems[i].title.c_str(), titleCol, 22);

            unsigned int descCol = isSelected ? GetColor(200, 220, 240) : GetColor(130, 145, 160);
            FontManager::GetInstance().DrawString(textX, iy + 44, sItems[i].desc.c_str(), descCol, 15);

            if (isSelected)
            {
                FontManager::GetInstance().DrawString(itemStartX + itemW - 35, iy + 24, "▶", sItems[i].isSpecial ? GetColor(255, 215, 0) : GetColor(0, 220, 255), 22);
            }
        }

        int footerY = panelY + panelH - 38;
        FontManager::GetInstance().DrawStringCenter(1920 / 2, footerY, "[ ↑ / ↓ / マウス ] 選択    [ Enter / クリック ] 決定・切替    [ ESC ] ポーズに戻る", GetColor(160, 190, 220), 16);
    }
    else if (state == GameState::CONTROLS)
    {
        // 操作説明オーバーレイ描画
        UI::ControlsGuide::Draw(1920, 1080);
    }
}



