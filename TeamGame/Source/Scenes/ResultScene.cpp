#include "ResultScene.h"
#include "../UI/HudStyle.h"
#include "../Managers/FontManager.h"
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
    if (resultBgIndex == -1) resultBgIndex = Load2DImage("Titleu.jpg");
    resultLogoIndex  = Load2DImage("result_logo.png");
    windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // UITextWindow によるリザルトパネルの初期化
    resultPanelWindow = new UI::UITextWindow();
    resultPanelWindow->SetFontHandle(FontManager::GetInstance().GetFont(22));
    resultPanelWindow->SetFixedSize(760, 300);
    resultPanelWindow->SetMaxTextWidth(680);
    resultPanelWindow->SetPadding(UI::Margin(44, 28, 44, 28));
    resultPanelWindow->SetLineSpacing(12);
    resultPanelWindow->SetBoxColor(HudStyle::ColBg(), HudStyle::ColBorder(), 225);
    if (windowFrameIndex != -1) {
        resultPanelWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }

    resultPanelWindow->SetTitle("BATTLE RESULT", HudStyle::ColAmber());
    resultPanelWindow->SetText("MATCH COMPLETED\n\n戦闘が終了しました。");
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

    bool anyConfirmHeld = (InputManager::GetInstance().IsKeyHeld(KEY_INPUT_RETURN) || 
                           InputManager::GetInstance().IsKeyHeld(KEY_INPUT_SPACE) || 
                           InputManager::GetInstance().IsMouseHeld(MOUSE_INPUT_LEFT));
    if (!anyConfirmHeld)
    {
        hasReleasedConfirm = true;
    }

    if (animTimer >= 0.5f && hasReleasedConfirm && 
        (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_RETURN) || 
         InputManager::GetInstance().IsKeyPressed(KEY_INPUT_SPACE) || 
         InputManager::GetInstance().IsMousePressed(MOUSE_INPUT_LEFT)))
    {
        SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
    }
}

void ResultScene::Draw()
{
    Scene::Draw();

    HudStyle::DrawTacticalBackdrop(1920, 1080, Get2DImage(resultBgIndex), 1920 / 2, 390, 480);

    const std::string& dispFont = FontManager::GetInstance().GetDisplayFontName();

    // ロゴ画像またはタイトル表示
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
        FontManager::GetInstance().DrawStringCenterShadow(1920 / 2, 134, "RESULT", HudStyle::ColText(), 54, -1, dispFont);
    }

    // リザルト中央パネル
    int panelW = 760;
    int panelX = (1920 - panelW) / 2;
    int panelY = 240;

    if (resultPanelWindow)
    {
        resultPanelWindow->Draw(panelX, panelY);
    }

    // ボタン案内
    int btnW = 400;
    int btnH = 52;
    int btnX = (1920 - btnW) / 2;
    int btnY = panelY + 340;

    HudStyle::DrawRoundPanel(btnX, btnY, btnW, btnH, HudStyle::RadiusButton, HudStyle::ColCard(), HudStyle::ColBorder(), 210);
    FontManager::GetInstance().DrawStringCenter(1920 / 2, btnY + 15, "[ ENTER / SPACE ] タイトルへ戻る", HudStyle::ColSubText(), 19);
}
