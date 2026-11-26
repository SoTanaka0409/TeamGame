#include "../UI/ControlsGuide.h"
#include "../UI/HudStyle.h"
#include "GameScene.h"
#include "Camera.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "SoundVisualizer.h"
#include "CutinManager.h"
#include "HitFeedbackManager.h"
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
    for (int i = 0; i < 6; i++)
    {
        if (pauseIconHandles[i] != -1)
        {
            DeleteGraph(pauseIconHandles[i]);
            pauseIconHandles[i] = -1;
        }
    }
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
    state = GameState::PLAYING;
    pauseMenuCursor = 0;
    prevEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    MatchManager::GetInstance().Reset();
    SoundVisualizer::GetInstance().Clear();
    HitFeedbackManager::GetInstance().Clear();
    CutinManager::GetInstance().Reset();
    prevMouseX = -1;
    prevMouseY = -1;

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

    // ポーズメニュー用アイコン画像の読み込み
    const char* pauseIconNames[6] = {
        "btn_resume.png",
        "btn_restart.png",
        "btn_settings.png",
        "btn_controls.png",
        "btn_title.png",
        "btn_exit.png"
    };
    for (int i = 0; i < 6; i++)
    {
        std::string p1 = std::string("Images/UI/Pause/") + pauseIconNames[i];
        std::string p2 = std::string("UI/Pause/") + pauseIconNames[i];
        std::string p3 = std::string("Resource/Images/UI/Pause/") + pauseIconNames[i];
        pauseIconHandles[i] = LoadGraphWithSearch(p1);
        if (pauseIconHandles[i] == -1) pauseIconHandles[i] = LoadGraphWithSearch(p2);
        if (pauseIconHandles[i] == -1) pauseIconHandles[i] = LoadGraphWithSearch(p3);
        if (pauseIconHandles[i] == -1) pauseIconHandles[i] = LoadGraphWithSearch(pauseIconNames[i]);
    }

    // タイトル・選択画面と共通の手描き質感UIパーツをロード
    uiModalImgIndex     = Load2DImage("Images/UI/PreGame/panel_modal.png");
    uiStripImgIndex     = Load2DImage("Images/UI/PreGame/panel_strip.png");
    uiBtnIdleImgIndex   = Load2DImage("Images/UI/PreGame/btn_idle.png");
    uiBtnSelImgIndex    = Load2DImage("Images/UI/PreGame/btn_selected.png");
    uiUnderlineImgIndex = Load2DImage("Images/UI/PreGame/header_underline.png");
}

void GameScene::Update()
{
    auto& input = InputManager::GetInstance();
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool isEscTriggered = (currEsc && !prevEsc) || input.IsKeyPressed(KEY_INPUT_ESCAPE) || input.IsPadStartPressed();
    bool isCancelTriggered = isEscTriggered || input.IsPadCancelPressed();
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currLeft = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0);
    bool currRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currClick = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;

    bool upPressed = (currUp && !prevUp) || input.IsPadUpPressed();
    bool downPressed = (currDown && !prevDown) || input.IsPadDownPressed();
    bool leftPressed = (currLeft && !prevLeft) || input.IsPadLeftPressed();
    bool rightPressed = (currRight && !prevRight) || input.IsPadRightPressed();
    bool confirmPressed = (currEnter && !prevEnter) || input.IsPadConfirmPressed();

    // ウィンドウ拡縮対応のマウス座標
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080))
    {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }
    bool mouseMoved = (prevMouseX != -1 && (mouseX != prevMouseX || mouseY != prevMouseY));

    if (state == GameState::PLAYING)
    {
        if (isEscTriggered)
        {
            state = GameState::PAUSED;
            pauseMenuCursor = 0;
            SetMouseDispFlag(TRUE);
        }
        else
        {
            // スタイリッシュカットイン演出の更新
            CutinManager::GetInstance().Update();

            if (introTimer > 0.0f)
            {
                introTimer -= 1.0f;
            }
            else if (CutinManager::GetInstance().ShouldPauseGame())
            {
                // カットイン演出の強烈なインパクト期間中はゲーム世界をヒットストップ
            }
            else if (HitFeedbackManager::GetInstance().ShouldSkipWorldUpdate())
            {
                // とどめを刺した直後の一瞬の停止・スロー演出（このフレームはゲーム世界を進めない）
            }
            else
            {
                gameTimer += 0.016f;
                Scene::Update();
            }

            // 当たった手ごたえ演出（画面揺れ・ダメージ数字など）のタイマー更新
            HitFeedbackManager::GetInstance().Update();

            // 3D空間オーディオのリスナー（プレイヤー）位置・向きの更新およびサウンド解放
            if (player && player->IsActive())
            {
                SoundManager::GetInstance().SetListener(player->GetPosition(), player->GetFacingDir());
            }
            SoundManager::GetInstance().Update();
            SoundVisualizer::GetInstance().Update();

            // 薄明イベント・定期音波ピンはイントロ終了後（戦闘開始後）からカウント進行
            if (introTimer <= 0.0f)
            {
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

#ifdef _DEBUG
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
#endif
        }
    }
    else if (state == GameState::PAUSED)
    {
        if (isCancelTriggered) state = GameState::PLAYING;

        if (upPressed)
        {
            pauseMenuCursor = (pauseMenuCursor + 4) % 6;
            SoundManager::GetInstance().PlayTapSE(0.4f);
        }
        else if (downPressed)
        {
            pauseMenuCursor = (pauseMenuCursor + 2) % 6;
            SoundManager::GetInstance().PlayTapSE(0.4f);
        }
        else if (leftPressed)
        {
            pauseMenuCursor = (pauseMenuCursor % 2 == 1) ? (pauseMenuCursor - 1) : (pauseMenuCursor + 1);
            SoundManager::GetInstance().PlayTapSE(0.4f);
        }
        else if (rightPressed)
        {
            pauseMenuCursor = (pauseMenuCursor % 2 == 0) ? (pauseMenuCursor + 1) : (pauseMenuCursor - 1);
            SoundManager::GetInstance().PlayTapSE(0.4f);
        }

        const int panelW = 1060;
        const int panelH = 660;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;
        const int colW = 460;
        const int rowH = 120;
        const int spacingX = 40;
        const int spacingY = 25;
        const int startX = panelX + (panelW - (colW * 2 + spacingX)) / 2;
        const int startY = panelY + 125;

        bool mouseClickedItem = false;
        for (int i = 0; i < 6; i++)
        {
            int r = i / 2;
            int c = i % 2;
            int bx = startX + c * (colW + spacingX);
            int by = startY + r * (rowH + spacingY);

            if (mouseX >= bx && mouseX <= bx + colW &&
                mouseY >= by && mouseY <= by + rowH)
            {
                if (mouseMoved && pauseMenuCursor != i)
                {
                    pauseMenuCursor = i;
                }
                if (currClick && !prevClick)
                {
                    pauseMenuCursor = i;
                    mouseClickedItem = true;
                }
            }
        }

        if (confirmPressed || mouseClickedItem)
        {
            SoundManager::GetInstance().PlayTapSE();
            if (pauseMenuCursor == 0) // ゲーム再開
            {
                state = GameState::PLAYING;
            }
            else if (pauseMenuCursor == 1) // やり直し
            {
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
            }
            else if (pauseMenuCursor == 2) // 各種設定
            {
                state = GameState::SETTINGS;
                settingsMenuCursor = 0;
            }
            else if (pauseMenuCursor == 3) // 操作説明
            {
                previousStateBeforeControls = GameState::PAUSED;
                state = GameState::CONTROLS;
            }
            else if (pauseMenuCursor == 4) // タイトルへ戻る
            {
                SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
            }
            else if (pauseMenuCursor == 5) // ゲーム終了
            {
                PostQuitMessage(0);
            }
        }
    }
    else if (state == GameState::SETTINGS)
    {
        if (isCancelTriggered) state = GameState::PAUSED;
        if (upPressed) settingsMenuCursor--;
        if (downPressed) settingsMenuCursor++;
        if (settingsMenuCursor < 0) settingsMenuCursor = 2;
        if (settingsMenuCursor > 2) settingsMenuCursor = 0;

        const int panelW = 680;
        const int panelH = 500;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;
        const int itemW = 580;
        const int itemH = 76;
        const int itemSpacing = 95;
        const int itemStartX = panelX + (panelW - itemW) / 2;
        const int itemStartY = panelY + 120; // Draw()の座標と完全一致

        bool mouseClickedItem = false;
        for (int i = 0; i < 3; i++)
        {
            int iy = itemStartY + i * itemSpacing;
            if (mouseX >= itemStartX && mouseX <= itemStartX + itemW &&
                mouseY >= iy && mouseY <= iy + itemH)
            {
                if (mouseMoved && settingsMenuCursor != i)
                {
                    settingsMenuCursor = i;
                }
                if (currClick && !prevClick)
                {
                    settingsMenuCursor = i;
                    mouseClickedItem = true;
                }
            }
        }

        if (confirmPressed || mouseClickedItem)
        {
            SoundManager::GetInstance().PlayTapSE();
            if (settingsMenuCursor == 0)
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (settingsMenuCursor == 1)
            {
                previousStateBeforeControls = GameState::SETTINGS;
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
        if (confirmPressed || isCancelTriggered || (currClick && !prevClick))
        {
            state = previousStateBeforeControls;
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
    prevMouseX = mouseX;
    prevMouseY = mouseY;
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

    // 当たった手ごたえの画面揺れ（描画のあいだだけカメラをずらし、Drawの最後で元に戻す）
    Vector2 shakeOffset = HitFeedbackManager::GetInstance().GetShakeOffset();
    float shakeZoom = (Camera::ZoomScale > 0.001f) ? Camera::ZoomScale : 1.0f;
    float shakeWorldX = shakeOffset.x / shakeZoom;
    float shakeWorldY = shakeOffset.y / shakeZoom;
    Camera::TargetWorldX += shakeWorldX;
    Camera::TargetWorldY += shakeWorldY;

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

    // 当たった手ごたえ: ヒットマーカー（×印）とダメージ数字（暗闇の上に出す）
    HitFeedbackManager::GetInstance().Draw();

    // 左上: 控えめなメニューキーバッジ
    HudStyle::DrawKeyHintItem(24, 16, "Esc", "メニュー");

    // デバッグ情報 (Theme / Variation) はデバッグモード時のみ表示
    if (DebugManager::GetInstance().IsDebugMode())
    {
        FontManager::GetInstance().DrawString(24, 48, (std::string("Theme: ") + std::to_string((int)stageManager.GetCurrentTheme() + 1)).c_str(), HudStyle::ColSubText(), 14);
        FontManager::GetInstance().DrawString(24, 66, (std::string("Variation: ") + std::to_string(stageManager.GetCurrentVariation() + 1)).c_str(), HudStyle::ColSubText(), 14);
    }

    // 画面上部中央: 統合キル数バー（タイトル・選択画面と共通の手描き質感プレート）
    int maxKills = MatchManager::GetInstance().GetTargetKillCount();
    int topPanelW = 680;
    int topPanelH = 38;
    int topPanelX = (1920 - topPanelW) / 2;
    int topPanelY = 14;

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
        int brH = 62;
        int brX = (1920 - brW) / 2;
        int brY = 14;

        float timeSec = GetNowCount() / 1000.0f;
        int pulseAlpha = static_cast<int>(210 + std::sin(timeSec * 12.0f) * 35);

        HudStyle::DrawTexturedPanel(brX, brY, brW, brH, Get2DImage(uiBtnSelImgIndex), HudStyle::ColBg(), HudStyle::ColEnemy(), pulseAlpha);

        FontManager::GetInstance().DrawStringCenterShadow(1920 / 2, brY + 7, "全員敵対・総力戦！", HudStyle::ColAmber(), 22, 8);

        char brSubText[64];
        snprintf(brSubText, sizeof(brSubText), "残り %.1f 秒", panicRemainingFrames / 60.0f);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, brY + 36, brSubText, HudStyle::ColText(), 16, 6);
    }
    else
    {
        HudStyle::DrawTexturedPanel(topPanelX, topPanelY, topPanelW, topPanelH, Get2DImage(uiStripImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 230);

        int team0Kills = MatchManager::GetInstance().GetTeam0Kills();
        int team1Kills = MatchManager::GetInstance().GetTeam1Kills();

        // 左: 味方キル数
        char allyText[64];
        snprintf(allyText, sizeof(allyText), "味方　%d / %d", team0Kills, maxKills);
        FontManager::GetInstance().DrawString(topPanelX + 20, topPanelY + 10, allyText, HudStyle::ColAlly(), 16, 7);

        // 右: 敵キル数
        char enemyText[64];
        snprintf(enemyText, sizeof(enemyText), "%d / %d　敵", team1Kills, maxKills);
        FontManager::GetInstance().DrawStringRight(topPanelX + topPanelW - 20, topPanelY + 10, enemyText, HudStyle::ColEnemy(), 16, 7);

        // 中央: 対向ゲージ (味方は中央から左へ、敵は中央から右へ)
        int centerX = 1920 / 2;
        int halfGaugeW = 140;
        int gaugeH = 7;
        int gaugeY = topPanelY + (topPanelH - gaugeH) / 2;

        float allyRatio = (maxKills > 0) ? static_cast<float>(team0Kills) / maxKills : 0.0f;
        float enemyRatio = (maxKills > 0) ? static_cast<float>(team1Kills) / maxKills : 0.0f;

        HudStyle::DrawSplitGauge(centerX - 4 - halfGaugeW, gaugeY, halfGaugeW, gaugeH, allyRatio, HudStyle::ColAlly(), true);
        HudStyle::DrawSplitGauge(centerX + 4, gaugeY, halfGaugeW, gaugeH, enemyRatio, HudStyle::ColEnemy(), false);

        DrawBox(centerX - 1, topPanelY + 9, centerX + 1, topPanelY + topPanelH - 9, HudStyle::ColBorder(), TRUE);
    }

    // 上部中央: 通知バナーの縦積みスタック（手描き質感ミニバー）
    int bannerStackY = (isBattleRoyaleActive ? 84 : topPanelY + topPanelH + 8);

    // 薄明イベントバナー
    if (brightTimer > 0.0f)
    {
        char moonBuf[64];
        snprintf(moonBuf, sizeof(moonBuf), "薄明発生中　残り %.1f秒", brightTimer / 60.0f);
        int bw = 300;
        int bh = 30;
        HudStyle::DrawTexturedPanel(1920 / 2 - bw / 2, bannerStackY, bw, bh, Get2DImage(uiBtnIdleImgIndex), HudStyle::ColBg(), HudStyle::ColAmberDim(), 225);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, bannerStackY + 7, moonBuf, HudStyle::ColAmber(), 15, 6);
        bannerStackY += bh + 6;
    }

    // 音波ピン探知バナー
    if (sonarNotificationTimer > 0.0f)
    {
        int bw = 260;
        int bh = 30;
        HudStyle::DrawTexturedPanel(1920 / 2 - bw / 2, bannerStackY, bw, bh, Get2DImage(uiBtnIdleImgIndex), HudStyle::ColBg(), HudStyle::ColAlly(), 225);
        FontManager::GetInstance().DrawStringCenter(1920 / 2, bannerStackY + 7, "音波探知発生！", HudStyle::ColAlly(), 15, 6);
        bannerStackY += bh + 6;
    }

    // 画面左下にプレイヤーのキャラクターHUD（HP・MP・武器）UIを表示
    if (player && player->IsActive()) {
        player->DrawUI(24, 855);
    }

    if (state == GameState::PLAYING && introTimer > 0.0f)
    {
        if (introTimer > 60.0f) {
            FontManager::GetInstance().DrawStringCenterShadow(1920 / 2, 1080 / 2 - 70, "まもなく開始...", HudStyle::ColAmber(), 62, 8);
        } else {
            HudStyle::DrawSectionHeading(1920 / 2, 1080 / 2 - 76, "戦闘開始！", Get2DImage(uiUnderlineImgIndex), 72);
        }
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
        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, 1920, 1080, GetColor(5, 6, 4), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインパネル枠（手描き質感）
        const int panelW = 1060;
        const int panelH = 660;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;

        HudStyle::DrawTexturedPanel(panelX, panelY, panelW, panelH, Get2DImage(uiModalImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 250);

        // ヘッダータイトル（日本語＋血痕ブラシ下線）
        int headerY = panelY + 32;
        HudStyle::DrawSectionHeading(1920 / 2, headerY, "ポーズ", Get2DImage(uiUnderlineImgIndex), 36);

        // 3. 各項目のカード (2列×3行グリッド)
        const int colW = 460;
        const int rowH = 120;
        const int spacingX = 40;
        const int spacingY = 25;
        const int startX = panelX + (panelW - (colW * 2 + spacingX)) / 2;
        const int startY = panelY + 125;

        struct MenuItemInfo {
            const char* titleJa;
            const char* desc;
            bool isAlert;
        };

        const MenuItemInfo items[6] = {
            { "ゲーム再開", "戦闘を続行します", false },
            { "やり直し", "ステージを最初からやり直します", false },
            { "各種設定", "エイム固定や操作説明", false },
            { "操作説明", "キーボード・マウス操作ガイド", false },
            { "タイトルへ", "タイトル画面へ戻ります", true },
            { "ゲーム終了", "ゲームを終了します", true }
        };

        for (int i = 0; i < 6; i++)
        {
            int r = i / 2;
            int c = i % 2;
            int bx = startX + c * (colW + spacingX);
            int by = startY + r * (rowH + spacingY);
            bool isSelected = (i == pauseMenuCursor);

            HudStyle::DrawTexturedPanel(bx, by, colW, rowH,
                                        Get2DImage(isSelected ? uiBtnSelImgIndex : uiBtnIdleImgIndex),
                                        isSelected ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                        isSelected ? (items[i].isAlert ? HudStyle::ColEnemy() : HudStyle::ColAmber()) : HudStyle::ColBorderSoft(),
                                        isSelected ? 248 : 210);

            // アイコン背景枠
            const int iconBoxSize = 84;
            int ibx = bx + 18;
            int iby = by + (rowH - iconBoxSize) / 2;
            DrawBox(ibx, iby, ibx + iconBoxSize, iby + iconBoxSize, HudStyle::ColCanvas(), TRUE);
            DrawBox(ibx, iby, ibx + iconBoxSize, iby + iconBoxSize, isSelected ? HudStyle::ColAmberDim() : HudStyle::ColBorderDim(), FALSE);

            if (pauseIconHandles[i] != -1)
            {
                int pad = 10;
                DrawExtendGraph(ibx + pad, iby + pad, ibx + iconBoxSize - pad, iby + iconBoxSize - pad, pauseIconHandles[i], TRUE);
            }

            // テキスト情報（英語併記を廃止してすっきり日本語のみ）
            int textX = ibx + iconBoxSize + 20;
            unsigned int titleCol = isSelected ? (items[i].isAlert ? HudStyle::ColEnemy() : HudStyle::ColAmber()) : HudStyle::ColText();
            FontManager::GetInstance().DrawString(textX, by + 26, items[i].titleJa, titleCol, 24, isSelected ? 7 : 5);

            unsigned int descCol = isSelected ? HudStyle::ColText() : HudStyle::ColSubText();
            FontManager::GetInstance().DrawString(textX, by + 64, items[i].desc, descCol, 15);
        }

        // フッター案内（控えめなキーバッジ）
        int footerY = panelY + panelH - 48;
        int hx = 640;
        hx += HudStyle::DrawKeyHintItem(hx, footerY, "↑↓←→ / マウス", "選ぶ") + 28;
        hx += HudStyle::DrawKeyHintItem(hx, footerY, "Enter / クリック", "決定") + 28;
        HudStyle::DrawKeyHintItem(hx, footerY, "Esc", "再開");
    }
    else if (state == GameState::SETTINGS)
    {
        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, 1920, 1080, GetColor(5, 6, 4), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインパネル枠（手描き質感）
        const int panelW = 680;
        const int panelH = 500;
        const int panelX = (1920 - panelW) / 2;
        const int panelY = (1080 - panelH) / 2;

        HudStyle::DrawTexturedPanel(panelX, panelY, panelW, panelH, Get2DImage(uiModalImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 250);

        int headerY = panelY + 32;
        HudStyle::DrawSectionHeading(1920 / 2, headerY, "設定", Get2DImage(uiUnderlineImgIndex), 36);

        const int itemW = 580;
        const int itemH = 76;
        const int itemSpacing = 95;
        const int itemStartX = panelX + (panelW - itemW) / 2;
        const int itemStartY = panelY + 120;

        std::string modeText = std::string("エイム固定（Eキー）：") + (GameSettings::GetInstance().isAimLockHoldMode ? "長押し" : "切り替え");
        std::string modeDesc = GameSettings::GetInstance().isAimLockHoldMode ?
            "Eキーを押している間だけエイムを固定" :
            "Eキーを押すたびにエイム固定を切り替え";

        struct SettingsItemInfo {
            std::string title;
            std::string desc;
            const char* badge;
        };

        SettingsItemInfo sItems[3] = {
            { modeText, modeDesc, "E" },
            { "操作説明を見る", "操作方法とルールを確認", "?" },
            { "ポーズへ戻る", "ポーズメニューへ戻る", "←" }
        };

        for (int i = 0; i < 3; i++)
        {
            int iy = itemStartY + i * itemSpacing;
            bool isSelected = (i == settingsMenuCursor);

            HudStyle::DrawTexturedPanel(itemStartX, iy, itemW, itemH,
                                        Get2DImage(isSelected ? uiBtnSelImgIndex : uiBtnIdleImgIndex),
                                        isSelected ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                        isSelected ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                        isSelected ? 248 : 210);

            int badgeX = itemStartX + 18;
            int badgeY = iy + 14;
            int badgeSize = 48;
            DrawBox(badgeX, badgeY, badgeX + badgeSize, badgeY + badgeSize, HudStyle::ColCanvas(), TRUE);
            DrawBox(badgeX, badgeY, badgeX + badgeSize, badgeY + badgeSize, isSelected ? HudStyle::ColAmberDim() : HudStyle::ColBorderDim(), FALSE);
            unsigned int badgeTextCol = isSelected ? HudStyle::ColAmber() : HudStyle::ColSubText();
            FontManager::GetInstance().DrawStringCenter(badgeX + badgeSize / 2, badgeY + 12, sItems[i].badge, badgeTextCol, 20, 7);

            int textX = badgeX + badgeSize + 18;
            unsigned int titleCol = isSelected ? HudStyle::ColAmber() : HudStyle::ColText();
            FontManager::GetInstance().DrawString(textX, iy + 14, sItems[i].title.c_str(), titleCol, 21, isSelected ? 7 : 5);

            unsigned int descCol = isSelected ? HudStyle::ColText() : HudStyle::ColSubText();
            FontManager::GetInstance().DrawString(textX, iy + 45, sItems[i].desc.c_str(), descCol, 15);
        }

        int footerY = panelY + panelH - 44;
        int hx = 685;
        hx += HudStyle::DrawKeyHintItem(hx, footerY, "↑ / ↓", "選ぶ") + 28;
        hx += HudStyle::DrawKeyHintItem(hx, footerY, "Enter / クリック", "決定") + 28;
        HudStyle::DrawKeyHintItem(hx, footerY, "Esc", "戻る");
    }
    else if (state == GameState::CONTROLS)
    {
        // 操作説明オーバーレイ描画
        UI::ControlsGuide::Draw(1920, 1080);
    }

    // 画面揺れで一時的にずらしたカメラを元に戻す（照準の計算に揺れが入らないようにする）
    Camera::TargetWorldX -= shakeWorldX;
    Camera::TargetWorldY -= shakeWorldY;
}



