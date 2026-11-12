#include "FontManager.h"
#include "GameOverScene.h"
#include "SoundManager.h"
#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene.h"
#include <cmath>

GameOverScene::GameOverScene()
    : menuCursor(0), animTimer(0.0f)
{
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    gameOverBgIndex = Load2DImage("gameover_bg.png");
    if (gameOverBgIndex == -1) {
        gameOverBgIndex = Load2DImage("result_gameover_bg.png");
    }
    windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // ゲームオーバーロゴ画像の読み込み
    gameOverLogoIndex = Load2DImage("logo_gameover.png");
    if (gameOverLogoIndex == -1) {
        gameOverLogoIndex = Load2DImage("UI/GameOver/logo_gameover.png");
    }
    if (gameOverLogoIndex == -1) {
        gameOverLogoIndex = Load2DImage("Images/UI/GameOver/logo_gameover.png");
    }

    // UITextWindow によるパネル枠の初期化
    defeatWindow = new UI::UITextWindow();
    defeatWindow->SetFixedSize(560, 260);
    defeatWindow->SetMaxTextWidth(500);
    defeatWindow->SetPadding(UI::Margin(35, 20, 35, 20));
    defeatWindow->SetBoxColor(GetColor(35, 10, 10), GetColor(200, 50, 50), 190);
    if (windowFrameIndex != -1) {
        defeatWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }
}

GameOverScene::~GameOverScene()
{
    delete defeatWindow;
    defeatWindow = nullptr;
}

void GameOverScene::Init()
{
    // 敗北音・ゲームオーバーBGMを再生
    SoundManager::GetInstance().PlayBGM("defeat_bgm", 0.85f);
    Scene::Init();
    animTimer = 0.0f;
    menuCursor = 0;
    prevEnter = true; // 前のシーンのボタン押しっぱなし事故防止
}

void GameOverScene::Update()
{
    Scene::Update();

    animTimer += 0.016f;

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnterKey = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);

    if (!currEnterKey && !currClick)
    {
        hasReleasedConfirm = true;
    }

    if (currUp && !prevUp)
    {
        menuCursor = (menuCursor - 1 + 2) % 2;
        SoundManager::GetInstance().PlayTapSE(0.4f);
    }
    if (currDown && !prevDown)
    {
        menuCursor = (menuCursor + 1) % 2;
        SoundManager::GetInstance().PlayTapSE(0.4f);
    }

    // マウスホバー＆クリック判定
    const int centerX = 1920 / 2;
    const int panelW = 620;
    const int panelH = 300;
    const int panelX = centerX - panelW / 2;
    const int logoH = 189;
    const int panelY = 160 + logoH + 65;
    const int btnW = 540;
    const int btnH = 80;
    const int btnX = panelX + (panelW - btnW) / 2;
    const int startBtnY = panelY + 45;
    const int spacing = 100;

    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    bool mouseClickedItem = false;

    for (int i = 0; i < 2; i++)
    {
        int by = startBtnY + i * spacing;
        if (mouseX >= btnX && mouseX <= btnX + btnW &&
            mouseY >= by && mouseY <= by + btnH)
        {
            if (menuCursor != i)
            {
                menuCursor = i;
            }
            if (currClick && !prevClick)
            {
                mouseClickedItem = true;
            }
        }
    }

    // シーン遷移後1.5秒間＆一度キーを離すまでは決定操作をロック（連打事故防止）
    bool canConfirm = (animTimer >= 1.5f) && hasReleasedConfirm;
    if (canConfirm && ((currEnterKey && !prevEnter) || mouseClickedItem))
    {
        SoundManager::GetInstance().PlayTapSE();
        if (menuCursor == 0)
        {
            // もう一度プレイ
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::SOLO));
        }
        else if (menuCursor == 1)
        {
            // タイトルへ戻る
            SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnterKey;
    prevClick = currClick;
}

void GameOverScene::Draw()
{
    Scene::Draw();

    // 背景描画（配列番号から呼び出し）
    if (gameOverBgIndex != -1)
    {
        DrawFullScreen2DImage(gameOverBgIndex, false);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(0, 0, 1920, 1080, GetColor(30, 5, 5), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // 暗い赤基調のグラデーション背景
        DrawBox(0, 0, 1920, 1080, GetColor(20, 5, 5), TRUE);
    }

    // 装飾背景ライン (赤色の警告ライン風 走査線アニメーション演出)
    for (int i = 0; i < 10; ++i)
    {
        int y = (i * 120 + static_cast<int>(animTimer * 40.0f)) % 1080;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 35);
        DrawLine(0, y, 1920, y, GetColor(255, 50, 50), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    const int centerX = 1920 / 2;

    // タイトルアニメーション (鼓動風の振動)
    float pulse = sinf(animTimer * 5.0f) * 6.0f;
    int logoW = 840;
    int logoH = 189; // 960x216 のアスペクト比維持
    int logoX = centerX - logoW / 2;
    int logoY = static_cast<int>(160 + pulse);

    int logoHandle = Get2DImage(gameOverLogoIndex);
    if (logoHandle != -1)
    {
        // 背後のおどろおどろしい赤黒いグロー光彩
        float glowPulse = std::abs(sinf(animTimer * 4.0f));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(45 + glowPulse * 35));
        DrawBox(logoX - 25, logoY - 15, logoX + logoW + 25, logoY + logoH + 15, GetColor(180, 20, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // GAME OVER ロゴ画像の描画
        DrawExtendGraph(logoX, logoY, logoX + logoW, logoY + logoH, logoHandle, TRUE);
    }
    else
    {
        // 画像未検出時のフォールバックテキスト
        FontManager::GetInstance().DrawStringCenter(centerX + 2, logoY + 4, "=== GAME OVER ===", GetColor(0, 0, 0), 40);
        FontManager::GetInstance().DrawStringCenter(centerX, logoY, "=== GAME OVER ===", GetColor(255, 60, 60), 40);
    }

    // パネル枠
    const int panelW = 620;
    const int panelH = 300;
    const int panelX = centerX - panelW / 2;
    const int panelY = 160 + logoH + 65;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(25, 8, 12), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 多重レッドネオン枠
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(220, 45, 45), FALSE);
    DrawBox(panelX + 2, panelY + 2, panelX + panelW - 2, panelY + panelH - 2, GetColor(140, 25, 25), FALSE);

    // メニュー選択肢カード
    struct GameOverOption {
        const char* titleJa;
        const char* titleEn;
    };
    const GameOverOption options[2] = {
        { "もう一度プレイ", "RETRY MATCH" },
        { "タイトル画面へ", "BACK TO TITLE" }
    };

    const int btnW = 540;
    const int btnH = 80;
    const int btnX = panelX + (panelW - btnW) / 2;
    const int startBtnY = panelY + 45;
    const int spacing = 100;

    for (int i = 0; i < 2; ++i)
    {
        int by = startBtnY + i * spacing;
        bool isSelected = (i == menuCursor);

        if (isSelected)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 240);
            DrawBox(btnX, by, btnX + btnW, by + btnH, GetColor(60, 20, 26), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            DrawBox(btnX, by, btnX + btnW, by + btnH, GetColor(255, 80, 80), FALSE);
            DrawBox(btnX + 1, by + 1, btnX + btnW - 1, by + btnH - 1, GetColor(255, 80, 80), FALSE);

            // 左端アクティブバー
            DrawBox(btnX, by, btnX + 6, by + btnH, GetColor(255, 80, 80), TRUE);

            // 選択矢印
            FontManager::GetInstance().DrawString(btnX + btnW - 35, by + 26, "▶", GetColor(255, 80, 80), 22);
        }
        else
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawBox(btnX, by, btnX + btnW, by + btnH, GetColor(20, 7, 10), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            DrawBox(btnX, by, btnX + btnW, by + btnH, GetColor(100, 35, 40), FALSE);
        }

        unsigned int titleCol = isSelected ? GetColor(255, 230, 230) : GetColor(180, 150, 150);
        FontManager::GetInstance().DrawString(btnX + 35, by + 24, options[i].titleJa, titleCol, 24);

        unsigned int enCol = isSelected ? GetColor(220, 110, 110) : GetColor(140, 85, 85);
        FontManager::GetInstance().DrawString(btnX + 230, by + 29, options[i].titleEn, enCol, 16);
    }

    if (animTimer >= 1.5f)
    {
        FontManager::GetInstance().DrawStringCenter(centerX, 880, "[ W / S / ↑ / ↓ / マウス ] 選択    [ Enter / クリック ] 決定", GetColor(180, 120, 120), 16);
    }
}
