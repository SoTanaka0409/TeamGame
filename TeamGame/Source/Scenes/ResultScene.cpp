#include "ResultScene.h"
#include "DxLib.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"

ResultScene::ResultScene()
{
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    resultBgIndex    = Load2DImage("result_bg.png");
    resultLogoIndex  = Load2DImage("result_logo.png");
    windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // UITextWindow によるリザルトパネルの初期化
    resultPanelWindow = new UI::UITextWindow();
    resultPanelWindow->SetFixedSize(800, 320);
    resultPanelWindow->SetMaxTextWidth(740);
    resultPanelWindow->SetPadding(UI::Margin(40, 25, 40, 25));
    resultPanelWindow->SetLineSpacing(12);
    resultPanelWindow->SetBoxColor(GetColor(15, 22, 35), GetColor(60, 90, 130), 190);
    if (windowFrameIndex != -1) {
        resultPanelWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }

    resultPanelWindow->SetTitle("BATTLE RESULT", GetColor(255, 220, 0));
    resultPanelWindow->SetText("MATCH COMPLETED\n\nThank you for playing!\n戦闘が終了しました。お疲れ様でした！");
}

ResultScene::~ResultScene()
{
    delete resultPanelWindow;
    resultPanelWindow = nullptr;
}

void ResultScene::Update()
{
    Scene::Update();

    animTimer += 0.016f; // ~60 FPS 走査線アニメーション更新

    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN) || InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
    }
}

void ResultScene::Draw()
{
    Scene::Draw();

    // 1. 背景画像の描画（配列番号から呼び出し）
    if (resultBgIndex != -1)
    {
        DrawFullScreen2DImage(resultBgIndex, false);
        // 背景画像の上に半透明の暗幕を重ねて文字を際立たせる
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);
    }

    // 装飾背景ライン (走査線アニメーション演出 - sou からの復元)
    for (int i = 0; i < 10; ++i)
    {
        int y = (i * 120 + static_cast<int>(animTimer * 50.0f)) % 1080;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 35);
        DrawLine(0, y, 1920, y, GetColor(0, 200, 255), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 2. リザルトロゴ/バナー画像の描画（配列番号から呼び出し）
    int logoHandle = Get2DImage(resultLogoIndex);
    if (logoHandle != -1)
    {
        int logoW = 0, logoH = 0;
        GetGraphSize(logoHandle, &logoW, &logoH);
        int logoX = (1920 - logoW) / 2;
        int logoY = 120;
        DrawGraph(logoX, logoY, logoHandle, TRUE);
    }
    else
    {
        DrawBox(1920 / 2 - 250, 120, 1920 / 2 + 250, 180, GetColor(20, 35, 55), TRUE);
        DrawBox(1920 / 2 - 250, 120, 1920 / 2 + 250, 180, GetColor(0, 200, 255), FALSE);
        DrawString(1920 / 2 - 80, 140, "=== RESULT ===", GetColor(255, 220, 0));
    }

    // 3. リザルト中央パネル（UITextWindow による描画）
    int panelW = 800;
    int panelX = (1920 - panelW) / 2;
    int panelY = 240;

    if (resultPanelWindow)
    {
        resultPanelWindow->Draw(panelX, panelY);
    }

    // ボタン案内
    int btnW = 400;
    int btnH = 50;
    int btnX = (1920 - btnW) / 2;
    int btnY = panelY + 340;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(btnX, btnY, btnX + btnW, btnY + btnH, GetColor(25, 40, 65), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(btnX, btnY, btnX + btnW, btnY + btnH, GetColor(0, 180, 255), FALSE);
    DrawString(btnX + 45, btnY + 16, "[ ENTER / SPACE ] タイトルへ戻る", GetColor(255, 255, 255));
}
