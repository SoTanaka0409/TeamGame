#include "ClearScene.h"
#include "../UI/HudStyle.h"
#include "../Managers/FontManager.h"
#include "SoundManager.h"
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
    if (clearBgIndex == -1) {
        clearBgIndex = Load2DImage("Titleu.jpg");
    }
    windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // ゲームクリアロゴ画像の読み込み
    clearLogoIndex = Load2DImage("logo_gameclear.png");
    if (clearLogoIndex == -1) {
        clearLogoIndex = Load2DImage("UI/Clear/logo_gameclear.png");
    }
    if (clearLogoIndex == -1) {
        clearLogoIndex = Load2DImage("Images/UI/Clear/logo_gameclear.png");
    }
    uiModalImgIndex   = Load2DImage("Images/UI/PreGame/panel_modal.png");
    uiBtnIdleImgIndex = Load2DImage("Images/UI/PreGame/btn_idle.png");
    uiBtnSelImgIndex  = Load2DImage("Images/UI/PreGame/btn_selected.png");

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
    // クリア音・ファンファーレBGMを再生
    SoundManager::GetInstance().PlayBGM("clear_bgm", 0.85f);
    Scene::Init();
    SetMouseDispFlag(TRUE);
    animTimer = 0.0f;
    menuCursor = 0;
    prevEnter = true;     // 前のシーンの押しっぱなし事故防止
    prevMouseLeft = true; // 前のシーンのクリック押しっぱなし事故防止
    prevMouseX = -1;
    prevMouseY = -1;
    hasReleasedConfirm = false;
}

void ClearScene::Update()
{
    Scene::Update();

    animTimer += 0.016f; // ~60 FPS timer update

    // マウス座標の取得（ウィンドウ拡縮対応）
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080))
    {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }

    auto& input = InputManager::GetInstance();
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnterKey = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currMouseLeft = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool mouseClicked = (currMouseLeft && !prevMouseLeft);
    bool mouseMoved = (prevMouseX != -1 && (mouseX != prevMouseX || mouseY != prevMouseY));

    if (!currEnterKey && !currMouseLeft && !input.IsPadConfirmHeld())
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

    // ボタンのマウスホバー＆クリック判定（Draw()の座標と完全一致）
    const int centerX = 1920 / 2;
    const int btnW = 380;
    const int btnH = 54;
    const int btnX = centerX - btnW / 2;
    const int menuStartY = 650;
    const int menuSpacing = 68;

    bool mouseClickedItem = false;
    for (int i = 0; i < 2; ++i)
    {
        int by = menuStartY + i * menuSpacing;
        if (mouseX >= btnX && mouseX <= btnX + btnW &&
            mouseY >= by && mouseY <= by + btnH)
        {
            if (mouseMoved && menuCursor != i)
            {
                menuCursor = i;
            }
            if (mouseClicked)
            {
                menuCursor = i;
                mouseClickedItem = true;
            }
        }
    }

    // シーン遷移直後の誤クリック防止（1秒＆一度離してから受付）
    bool canConfirm = (animTimer >= 1.0f) && hasReleasedConfirm;
    if (canConfirm && ((currEnterKey && !prevEnter) || input.IsPadConfirmPressed() || mouseClickedItem))
    {
        SoundManager::GetInstance().PlayTapSE();
        if (menuCursor == 0)
        {
            // もう一度プレイ
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
            prevUp = currUp;
            prevDown = currDown;
            prevEnter = currEnterKey;
            prevMouseLeft = currMouseLeft;
            prevMouseX = mouseX;
            prevMouseY = mouseY;
            return;
        }
        else if (menuCursor == 1)
        {
            // タイトルへ戻る
            SceneManager::GetInstance().ChangeScene(std::make_shared<TitleScene>());
            prevUp = currUp;
            prevDown = currDown;
            prevEnter = currEnterKey;
            prevMouseLeft = currMouseLeft;
            prevMouseX = mouseX;
            prevMouseY = mouseY;
            return;
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnterKey;
    prevMouseLeft = currMouseLeft;
    prevMouseX = mouseX;
    prevMouseY = mouseY;
}

void ClearScene::Draw()
{
    Scene::Draw();

    // タクティカル・ステルス背景＋中央スポットライト
    HudStyle::DrawTacticalBackdrop(1920, 1080, Get2DImage(clearBgIndex), 1920 / 2, 420, 500);

    const int centerX = 1920 / 2;
    const std::string& dispFont = FontManager::GetInstance().GetDisplayFontName();

    // タイトルアニメーション (優雅な浮遊＆脈動エフェクト)
    float pulse = sinf(animTimer * 3.0f) * 3.0f;
    int logoW = 720;
    int logoH = 162;
    int logoX = centerX - logoW / 2;
    int logoY = static_cast<int>(54 + pulse);

    int logoHandle = Get2DImage(clearLogoIndex);
    if (logoHandle != -1)
    {
        float glowPulse = std::abs(sinf(animTimer * 2.5f));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(20 + glowPulse * 20));
        HudStyle::DrawRoundBox(logoX - 25, logoY - 12, logoX + logoW + 25, logoY + logoH + 12, HudStyle::RadiusModal, HudStyle::ColAmber(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawExtendGraph(logoX, logoY, logoX + logoW, logoY + logoH, logoHandle, TRUE);

        if (stats.rankName == "1P WIN" || stats.rankName == "2P WIN")
        {
            const char* subText = (stats.rankName == "1P WIN") ? "1P VICTORY" : "2P VICTORY";
            unsigned int subCol = (stats.rankName == "1P WIN") ? HudStyle::ColAlly() : HudStyle::ColEnemy();
            FontManager::GetInstance().DrawStringCenterShadow(centerX, logoY + logoH + 6, subText, subCol, 24, -1, dispFont);
        }
    }
    else
    {
        std::string headerText = "STAGE CLEAR";
        unsigned int headerColor = HudStyle::ColAmber();
        if (stats.rankName == "1P WIN") { headerText = "1P VICTORY"; headerColor = HudStyle::ColAlly(); }
        else if (stats.rankName == "2P WIN") { headerText = "2P VICTORY"; headerColor = HudStyle::ColEnemy(); }

        FontManager::GetInstance().DrawStringCenterShadow(centerX, logoY + 20, headerText.c_str(), headerColor, 56, -1, dispFont);
    }

    // リザルトパネル（手描き質感）
    int panelW = 680;
    int panelH = 350;
    int panelX = centerX - panelW / 2;
    int panelY = 250;

    HudStyle::DrawTexturedPanel(panelX, panelY, panelW, panelH, Get2DImage(uiModalImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 245);

    FontManager::GetInstance().DrawStringCenter(centerX, panelY + 26, "戦績", HudStyle::ColAmber(), 26, 7);
    DrawLine(panelX + 50, panelY + 62, panelX + panelW - 50, panelY + 62, HudStyle::ColBorderSoft());

    int itemStartY = panelY + 86;
    int itemStep = 56;
    int labelX = panelX + 80;
    int valRightX = panelX + panelW - 80;

    int minutes = static_cast<int>(stats.clearTimeSec) / 60;
    float seconds = fmodf(stats.clearTimeSec, 60.0f);
    char timeVal[32], enemyVal[32], scoreVal[32];
    snprintf(timeVal, sizeof(timeVal), "%02d:%05.2f", minutes, seconds);
    snprintf(enemyVal, sizeof(enemyVal), "%d / %d", stats.defeatedEnemies, stats.totalEnemies);
    snprintf(scoreVal, sizeof(scoreVal), "%d 点", stats.rankScore);

    FontManager::GetInstance().DrawString(labelX, itemStartY + 0 * itemStep, "クリアタイム", HudStyle::ColSubText(), 19);
    FontManager::GetInstance().DrawStringRight(valRightX, itemStartY + 0 * itemStep - 2, timeVal, HudStyle::ColText(), 24, 7);

    FontManager::GetInstance().DrawString(labelX, itemStartY + 1 * itemStep, "撃破数", HudStyle::ColSubText(), 19);
    FontManager::GetInstance().DrawStringRight(valRightX, itemStartY + 1 * itemStep - 2, enemyVal, HudStyle::ColText(), 24, 7);

    FontManager::GetInstance().DrawString(labelX, itemStartY + 2 * itemStep, "スコア", HudStyle::ColSubText(), 19);
    FontManager::GetInstance().DrawStringRight(valRightX, itemStartY + 2 * itemStep - 2, scoreVal, HudStyle::ColText(), 24, 7);

    FontManager::GetInstance().DrawString(labelX, itemStartY + 3 * itemStep + 4, "評価ランク", HudStyle::ColSubText(), 19);
    unsigned int rankColor = HudStyle::ColAmber();
    if (stats.rankName == "A") rankColor = HudStyle::ColAlly();
    else if (stats.rankName == "B") rankColor = HudStyle::ColText();
    else if (stats.rankName == "C") rankColor = HudStyle::ColSubText();
    FontManager::GetInstance().DrawStringRight(valRightX, itemStartY + 3 * itemStep - 8, stats.rankName.c_str(), rankColor, 38, 8);

    // メニュー選択（手描き質感ボタン）
    const char* options[] = {
        "もう一度プレイ",
        "タイトルへ戻る"
    };

    int btnW = 380;
    int btnH = 54;
    int btnX = centerX - btnW / 2;
    int menuStartY = 650;
    int menuSpacing = 68;

    for (int i = 0; i < 2; ++i)
    {
        int by = menuStartY + i * menuSpacing;
        bool isSel = (i == menuCursor);
        HudStyle::DrawTexturedPanel(btnX, by, btnW, btnH,
                                    Get2DImage(isSel ? uiBtnSelImgIndex : uiBtnIdleImgIndex),
                                    isSel ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                    isSel ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                    isSel ? 245 : 205);
        FontManager::GetInstance().DrawStringCenter(centerX, by + 16, options[i], isSel ? HudStyle::ColAmber() : HudStyle::ColSubText(), 20, isSel ? 7 : 5);
    }

    if (animTimer >= 1.0f)
    {
        int hx = 770;
        hx += HudStyle::DrawKeyHintItem(hx, 825, "W / S", "選ぶ") + 28;
        HudStyle::DrawKeyHintItem(hx, 825, "Enter / クリック", "決定");
    }
}
