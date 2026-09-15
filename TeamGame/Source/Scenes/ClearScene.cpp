#include "ClearScene.h"
#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene.h"
#include <cstdio>
#include <cmath>

ClearScene::ClearScene(const ClearStats& inStats)
    : stats(inStats), menuCursor(0), animTimer(0.0f)
{
}

ClearScene::~ClearScene()
{
}

void ClearScene::Init()
{
    Scene::Init();
    animTimer = 0.0f;
}

void ClearScene::Update()
{
    Scene::Update();

    animTimer += 0.016f; // ~60 FPS timer update

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);

    if (currUp && !prevUp)
    {
        menuCursor = (menuCursor - 1 + 2) % 2;
    }
    if (currDown && !prevDown)
    {
        menuCursor = (menuCursor + 1) % 2;
    }

    if (currEnter && !prevEnter)
    {
        if (menuCursor == 0)
        {
            // もう一度プレイ
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
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

void ClearScene::Draw()
{
    Scene::Draw();

    // 暗いグラデーション背景
    DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);

    // 装飾背景ライン
    for (int i = 0; i < 10; ++i)
    {
        int y = (i * 120 + static_cast<int>(animTimer * 50.0f)) % 1080;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
        DrawLine(0, y, 1920, y, GetColor(0, 200, 255), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    const int centerX = 1920 / 2;

    // タイトルアニメーション (拡大＆波打つエフェクト)
    float pulse = sinf(animTimer * 4.0f) * 4.0f;
    int titleY = static_cast<int>(180 + pulse);

    // タイトル文字シャドウ
    DrawString(centerX - 198, titleY + 4, "=== STAGE CLEAR! ===", GetColor(0, 0, 0));
    // メインタイトル文字
    DrawString(centerX - 200, titleY, "=== STAGE CLEAR! ===", GetColor(255, 215, 0));

    // リザルト枠
    int panelW = 600;
    int panelH = 340;
    int panelX = centerX - panelW / 2;
    int panelY = 320;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(20, 30, 45), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 200, 255), FALSE);

    // スコア＆統計情報表示
    int minutes = static_cast<int>(stats.clearTimeSec) / 60;
    float seconds = fmodf(stats.clearTimeSec, 60.0f);
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "クリアタイム : %02d:%05.2f", minutes, seconds);

    char enemyStr[64];
    snprintf(enemyStr, sizeof(enemyStr), "撃破数      : %d / %d", stats.defeatedEnemies, stats.totalEnemies);

    char scoreStr[64];
    snprintf(scoreStr, sizeof(scoreStr), "最終スコア  : %d pts", stats.rankScore);

    DrawString(panelX + 60, panelY + 50, timeStr, GetColor(255, 255, 255));
    DrawString(panelX + 60, panelY + 110, enemyStr, GetColor(255, 255, 255));
    DrawString(panelX + 60, panelY + 170, scoreStr, GetColor(255, 255, 255));

    // ランク表示
    DrawString(panelX + 60, panelY + 240, "評価ランク  :", GetColor(255, 255, 255));

    unsigned int rankColor = GetColor(255, 215, 0); // Gold for S
    if (stats.rankName == "A") rankColor = GetColor(200, 200, 255);
    else if (stats.rankName == "B") rankColor = GetColor(100, 255, 100);
    else if (stats.rankName == "C") rankColor = GetColor(200, 150, 100);

    DrawString(panelX + 220, panelY + 235, stats.rankName.c_str(), rankColor);

    // メニュー選択
    const char* options[] = {
        "もう一度プレイ (RETRY)",
        "タイトル画面へ (TITLE)"
    };

    int menuStartY = 730;
    int menuSpacing = 60;

    for (int i = 0; i < 2; ++i)
    {
        int itemY = menuStartY + i * menuSpacing;
        unsigned int color = (i == menuCursor) ? GetColor(255, 255, 0) : GetColor(180, 180, 180);

        if (i == menuCursor)
        {
            // カーソル矢印
            float arrowOffset = sinf(animTimer * 8.0f) * 5.0f;
            DrawString(static_cast<int>(centerX - 160 + arrowOffset), itemY, ">", color);
        }

        DrawString(centerX - 130, itemY, options[i], color);
    }

    DrawString(centerX - 160, 920, "[ W / S ] 選択   [ Enter ] 決定", GetColor(120, 150, 180));
}
