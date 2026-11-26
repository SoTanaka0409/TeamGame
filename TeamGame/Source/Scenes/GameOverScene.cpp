#include "FontManager.h"
#include "GameOverScene.h"
#include "../UI/HudStyle.h"
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
    if (gameOverBgIndex == -1) {
        gameOverBgIndex = Load2DImage("Titleu.jpg");
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
    uiModalImgIndex   = Load2DImage("Images/UI/PreGame/panel_modal.png");
    uiBtnIdleImgIndex = Load2DImage("Images/UI/PreGame/btn_idle.png");
    uiBtnSelImgIndex  = Load2DImage("Images/UI/PreGame/btn_selected.png");

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
    SetMouseDispFlag(TRUE);
    animTimer = 0.0f;
    menuCursor = 0;
    prevEnter = true; // 前のシーンのボタン押しっぱなし事故防止
    prevClick = true;
    prevMouseX = -1;
    prevMouseY = -1;
    hasReleasedConfirm = false;
}

void GameOverScene::Update()
{
    Scene::Update();

    animTimer += 0.016f;

    auto& input = InputManager::GetInstance();
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnterKey = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);

    if (!currEnterKey && !currClick && !input.IsPadConfirmHeld())
    {
        hasReleasedConfirm = true;
    }

    if ((currUp && !prevUp) || input.IsPadUpPressed())
    {
        menuCursor = (menuCursor - 1 + 2) % 2;
        SoundManager::GetInstance().PlayTapSE(0.4f);
    }
    if ((currDown && !prevDown) || input.IsPadDownPressed())
    {
        menuCursor = (menuCursor + 1) % 2;
        SoundManager::GetInstance().PlayTapSE(0.4f);
    }

    // マウスホバー＆クリック判定（ウィンドウ拡縮対応）
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
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080))
    {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }
    bool mouseMoved = (prevMouseX != -1 && (mouseX != prevMouseX || mouseY != prevMouseY));
    bool mouseClickedItem = false;

    for (int i = 0; i < 2; i++)
    {
        int by = startBtnY + i * spacing;
        if (mouseX >= btnX && mouseX <= btnX + btnW &&
            mouseY >= by && mouseY <= by + btnH)
        {
            if (mouseMoved && menuCursor != i)
            {
                menuCursor = i;
            }
            if (currClick && !prevClick)
            {
                menuCursor = i;
                mouseClickedItem = true;
            }
        }
    }

    // シーン遷移後1.0秒間＆一度キーを離すまでは決定操作をロック（連打事故防止）
    bool canConfirm = (animTimer >= 1.0f) && hasReleasedConfirm;
    if (canConfirm && ((currEnterKey && !prevEnter) || input.IsPadConfirmPressed() || mouseClickedItem))
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
    prevMouseX = mouseX;
    prevMouseY = mouseY;
}

void GameOverScene::Draw()
{
    Scene::Draw();

    // タクティカル・ステルス暗黒背景＋赤みのあるスポットライト
    HudStyle::DrawTacticalBackdrop(1920, 1080, Get2DImage(gameOverBgIndex), 1920 / 2, 540, 500);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
    DrawBox(0, 0, 1920, 1080, GetColor(40, 6, 8), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const int centerX = 1920 / 2;
    const std::string& dispFont = FontManager::GetInstance().GetDisplayFontName();

    // タイトルアニメーション (控えめな鼓動)
    float pulse = sinf(animTimer * 4.0f) * 4.0f;
    int logoW = 840;
    int logoH = 189;
    int logoX = centerX - logoW / 2;
    int logoY = static_cast<int>(160 + pulse);

    int logoHandle = Get2DImage(gameOverLogoIndex);
    if (logoHandle != -1)
    {
        float glowPulse = std::abs(sinf(animTimer * 3.0f));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(25 + glowPulse * 25));
        HudStyle::DrawRoundBox(logoX - 25, logoY - 15, logoX + logoW + 25, logoY + logoH + 15, HudStyle::RadiusModal, HudStyle::ColEnemy(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawExtendGraph(logoX, logoY, logoX + logoW, logoY + logoH, logoHandle, TRUE);
    }
    else
    {
        FontManager::GetInstance().DrawStringCenterShadow(centerX, logoY + 20, "GAME OVER", HudStyle::ColEnemy(), 64, -1, dispFont);
    }

    // パネル枠（手描き質感）
    const int panelW = 620;
    const int panelH = 300;
    const int panelX = centerX - panelW / 2;
    const int panelY = 160 + logoH + 65;

    HudStyle::DrawTexturedPanel(panelX, panelY, panelW, panelH, Get2DImage(uiModalImgIndex), HudStyle::ColBg(), HudStyle::ColEnemy(), 245);

    const char* options[2] = {
        "もう一度プレイ",
        "タイトルへ戻る"
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

        HudStyle::DrawTexturedPanel(btnX, by, btnW, btnH,
                                    Get2DImage(isSelected ? uiBtnSelImgIndex : uiBtnIdleImgIndex),
                                    isSelected ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSelected ? HudStyle::ColEnemy() : HudStyle::ColBorderSoft(),
                                    isSelected ? 248 : 205);

        unsigned int titleCol = isSelected ? HudStyle::ColAmber() : HudStyle::ColSubText();
        FontManager::GetInstance().DrawStringCenter(centerX, by + 27, options[i], titleCol, 24, isSelected ? 7 : 5);
    }

    if (animTimer >= 1.0f)
    {
        int hx = 770;
        hx += HudStyle::DrawKeyHintItem(hx, 880, "W / S", "選ぶ") + 28;
        HudStyle::DrawKeyHintItem(hx, 880, "Enter / クリック", "決定");
    }
}
