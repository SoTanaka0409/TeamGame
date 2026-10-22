#include "GameOverScene.h"
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
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0 || (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);

    if (currUp && !prevUp)
    {
        menuCursor = (menuCursor - 1 + 2) % 2;
    }
    if (currDown && !prevDown)
    {
        menuCursor = (menuCursor + 1) % 2;
    }

    // シーン遷移後2秒間は決定操作をロック（連打・押しっぱなしによる意図しない即時選択を防止）
    bool canConfirm = (animTimer >= 2.0f);
    if (canConfirm && currEnter && !prevEnter)
    {
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
    prevEnter = currEnter;
}

void GameOverScene::Draw()
{
    Scene::Draw();

    // 背景描画（配列番号から呼び出し）
    if (gameOverBgIndex != -1)
    {
        DrawFullScreen2DImage(gameOverBgIndex, false);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
        DrawBox(0, 0, 1920, 1080, GetColor(30, 5, 5), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // 暗い赤基調のグラデーション背景
        DrawBox(0, 0, 1920, 1080, GetColor(20, 5, 5), TRUE);

        // 装飾背景ライン (赤色の警告ライン風)
        for (int i = 0; i < 10; ++i)
        {
            int y = (i * 120 + static_cast<int>(animTimer * 40.0f)) % 1080;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
            DrawLine(0, y, 1920, y, GetColor(255, 50, 50), 1);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    const int centerX = 1920 / 2;

    // タイトルアニメーション (鼓動風の振動)
    float pulse = sinf(animTimer * 5.0f) * 5.0f;
    int titleY = static_cast<int>(250 + pulse);

    // 影
    DrawString(centerX - 178, titleY + 4, "=== GAME OVER ===", GetColor(0, 0, 0));
    // メインテキスト (赤色)
    DrawString(centerX - 180, titleY, "=== GAME OVER ===", GetColor(255, 60, 60));

    // パネル枠（UITextWindow による描画）
    int panelW = 560;
    int panelH = 260;
    int panelX = centerX - panelW / 2;
    int panelY = 410;

    if (defeatWindow)
    {
        defeatWindow->Draw(panelX, panelY);
    }
    else
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(35, 10, 10), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(200, 50, 50), FALSE);
    }

    // メニュー選択肢
    const char* options[] = {
        "もう一度プレイ (RETRY)",
        "タイトル画面へ (TITLE)"
    };

    int menuStartY = panelY + 60;
    int menuSpacing = 70;

    for (int i = 0; i < 2; ++i)
    {
        int itemY = menuStartY + i * menuSpacing;
        unsigned int color = (i == menuCursor) ? GetColor(255, 220, 0) : GetColor(180, 150, 150);

        if (i == menuCursor)
        {
            float arrowOffset = sinf(animTimer * 8.0f) * 5.0f;
            DrawString(static_cast<int>(centerX - 150 + arrowOffset), itemY, ">", color);
        }

        DrawString(centerX - 120, itemY, options[i], color);
    }

    if (animTimer >= 2.0f)
    {
        DrawString(centerX - 160, 850, "[ W / S ] 選択   [ Enter / クリック ] 決定", GetColor(180, 120, 120));
    }
}
