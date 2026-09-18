#include "GameOverScene.h"
#include "DxLib.h"
#include "InputManager.h"
#include "DebugManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene.h"
#include <cmath>

GameOverScene::GameOverScene()
    : menuCursor(0), animTimer(0.0f)
{
}

GameOverScene::~GameOverScene()
{
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

    const int centerX = 1920 / 2;

    // タイトルアニメーション (鼓動風の振動)
    float pulse = sinf(animTimer * 5.0f) * 5.0f;
    int titleY = static_cast<int>(250 + pulse);

    // 影
    DrawString(centerX - 178, titleY + 4, "=== GAME OVER ===", GetColor(0, 0, 0));
    // メインテキスト (赤色)
    DrawString(centerX - 180, titleY, "=== GAME OVER ===", GetColor(255, 60, 60));

    // パネル枠
    int panelW = 500;
    int panelH = 250;
    int panelX = centerX - panelW / 2;
    int panelY = 420;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(35, 10, 10), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(200, 50, 50), FALSE);

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

    if (animTimer < 2.0f)
    {
        if (DebugManager::GetInstance().IsDebugMode())
        {
            char waitMsg[64];
            snprintf(waitMsg, sizeof(waitMsg), "[ DEBUG: 決定操作ロック中... (あと %.1f 秒) ]", 2.0f - animTimer);
            DrawString(centerX - 210, 850, waitMsg, GetColor(255, 120, 120));
        }
    }
    else
    {
        DrawString(centerX - 160, 850, "[ W / S ] 選択   [ Enter / クリック ] 決定", GetColor(180, 120, 120));
    }
}
