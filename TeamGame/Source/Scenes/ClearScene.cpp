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
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    clearBgIndex = Load2DImage("clear_bg.png");
    if (clearBgIndex == -1) {
        clearBgIndex = Load2DImage("result_clear_bg.png");
    }
    windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // UITextWindow によるリザルト枠の初期化
    statsWindow = new UI::UITextWindow();
    statsWindow->SetFixedSize(700, 380);
    statsWindow->SetMaxTextWidth(640);
    statsWindow->SetPadding(UI::Margin(40, 25, 40, 25));
    statsWindow->SetLineSpacing(14);
    statsWindow->SetBoxColor(GetColor(20, 30, 45), GetColor(0, 200, 255), 190);
    if (windowFrameIndex != -1) {
        statsWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }
}

ClearScene::~ClearScene()
{
    delete statsWindow;
    statsWindow = nullptr;
}

void ClearScene::Init()
{
    Scene::Init();
    animTimer = 0.0f;
    prevEnter = true; // 前のシーンの押しっぱなし事故防止
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

    // シーン遷移後2秒間は決定操作をロック（連打・押しっぱなしによる意図しない即時選択を防止）
    bool canConfirm = (animTimer >= 2.0f);
    if (canConfirm && currEnter && !prevEnter)
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

    // 背景描画（配列番号から呼び出し）
    if (clearBgIndex != -1)
    {
        DrawFullScreen2DImage(clearBgIndex, false);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // 画像がない場合は暗いグラデーション背景
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);

        // 装飾背景ライン
        for (int i = 0; i < 10; ++i)
        {
            int y = (i * 120 + static_cast<int>(animTimer * 50.0f)) % 1080;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
            DrawLine(0, y, 1920, y, GetColor(0, 200, 255), 1);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    const int centerX = 1920 / 2;

    // タイトルアニメーション (拡大＆波打つエフェクト)
    float pulse = sinf(animTimer * 4.0f) * 4.0f;
    int titleY = static_cast<int>(150 + pulse);

    SetFontSize(48);
    std::string headerText = "STAGE CLEAR!";
    unsigned int headerColor = GetColor(255, 215, 0);
    if (stats.rankName == "1P WIN")
    {
        headerText = "1P (青) VICTORY!";
        headerColor = GetColor(80, 200, 255);
    }
    else if (stats.rankName == "2P WIN")
    {
        headerText = "2P (赤) VICTORY!";
        headerColor = GetColor(255, 80, 80);
    }

    int titleWidth = GetDrawStringWidth(headerText.c_str(), -1);
    // タイトル文字シャドウ
    DrawString(centerX - titleWidth / 2 + 4, titleY + 4, headerText.c_str(), GetColor(0, 0, 0));
    // メインタイトル文字
    DrawString(centerX - titleWidth / 2, titleY, headerText.c_str(), headerColor);

    // リザルト枠（UITextWindow による描画）
    int panelW = 700;
    int panelH = 380;
    int panelX = centerX - panelW / 2;
    int panelY = 240;

    if (statsWindow)
    {
        statsWindow->SetTitle("STAGE RESULT", GetColor(255, 220, 0));
        statsWindow->Draw(panelX, panelY);
    }
    else
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(20, 30, 45), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 200, 255), FALSE);
    }

    // スコア＆統計情報表示
    SetFontSize(24);
    int minutes = static_cast<int>(stats.clearTimeSec) / 60;
    float seconds = fmodf(stats.clearTimeSec, 60.0f);
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "クリアタイム : %02d:%05.2f", minutes, seconds);

    char enemyStr[64];
    snprintf(enemyStr, sizeof(enemyStr), "撃破数       : %d / %d", stats.defeatedEnemies, stats.totalEnemies);

    char scoreStr[64];
    snprintf(scoreStr, sizeof(scoreStr), "最終スコア   : %d pts", stats.rankScore);

    DrawString(panelX + 80, panelY + 45, timeStr, GetColor(255, 255, 255));
    DrawString(panelX + 80, panelY + 115, enemyStr, GetColor(255, 255, 255));
    DrawString(panelX + 80, panelY + 185, scoreStr, GetColor(255, 255, 255));

    // ランク表示（「評価ランク : 」と ランク大文字 "S" などの横位置・縦位置を精密に揃える）
    const char* rankLabel = "評価ランク   : ";
    DrawString(panelX + 80, panelY + 260, rankLabel, GetColor(255, 255, 255));

    int labelWidth = GetDrawStringWidth(rankLabel, -1);

    unsigned int rankColor = GetColor(255, 215, 0); // Gold for S
    if (stats.rankName == "A") rankColor = GetColor(200, 200, 255);
    else if (stats.rankName == "B") rankColor = GetColor(100, 255, 100);
    else if (stats.rankName == "C") rankColor = GetColor(200, 150, 100);

    // ランク文字を強調大文字フォント(40pt)で位置調整して描画
    SetFontSize(40);
    DrawString(panelX + 80 + labelWidth + 5, panelY + 248, stats.rankName.c_str(), rankColor);

    // メニュー選択
    SetFontSize(28);
    const char* options[] = {
        "もう一度プレイ (RETRY)",
        "タイトル画面へ (TITLE)"
    };

    int menuStartY = 670;
    int menuSpacing = 60;

    for (int i = 0; i < 2; ++i)
    {
        int itemY = menuStartY + i * menuSpacing;
        unsigned int color = (i == menuCursor) ? GetColor(255, 255, 0) : GetColor(180, 180, 180);
        int optWidth = GetDrawStringWidth(options[i], -1);
        int optX = centerX - optWidth / 2;

        if (i == menuCursor)
        {
            // カーソル矢印
            float arrowOffset = sinf(animTimer * 8.0f) * 5.0f;
            DrawString(static_cast<int>(optX - 35 + arrowOffset), itemY, ">", color);
        }

        DrawString(optX, itemY, options[i], color);
    }

    if (animTimer >= 2.0f)
    {
        SetFontSize(22);
        const char* guideMsg = "[ W / S / 矢印キー ] 選択   [ Enter / Space / 左クリック ] 決定";
        int msgWidth = GetDrawStringWidth(guideMsg, -1);
        DrawString(centerX - msgWidth / 2, 820, guideMsg, GetColor(140, 180, 220));
    }

    SetFontSize(32); // フォントサイズをデフォルトに戻す
}
