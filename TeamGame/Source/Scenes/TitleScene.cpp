#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "GameSettings.h"
#include "../UI/ControlsGuide.h"
#include "../UI/HudStyle.h"
#include "../Managers/FontManager.h"
#include <string>
#include <random>



TitleScene::TitleScene()
{
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理 - miuraのタイトル画像設定】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    titleBgIndex       = Load2DImage("Titleu.jpg");
    if (titleBgIndex == -1) titleBgIndex = Load2DImage("title_bg.png");
    titleLogoIndex     = Load2DImage("Title02.png");
    if (titleLogoIndex == -1) titleLogoIndex = Load2DImage("Title.png");
    titleLogoIndex2    = Load2DImage("Title2.png");
    settingsBgIndex    = Load2DImage("settings_bg.png");
    modeSelectBgIndex  = Load2DImage("mode_select_bg.png");
    windowFrameIndex   = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // 新規タイトルメニューグラフィック
    btnStartImgIndex    = Load2DImage("Title_GameStart.png");
    btnSettingsImgIndex = Load2DImage("Title_Settings.png");
    btnExitImgIndex     = Load2DImage("Title_Exit.png");
    btnBannerImgIndex   = Load2DImage("Title_BloodBanner.png");

    // 手描き・ホラー質感UIパーツ
    uiModalImgIndex     = Load2DImage("Images/UI/PreGame/panel_modal.png");
    uiStripImgIndex     = Load2DImage("Images/UI/PreGame/panel_strip.png");
    uiBtnIdleImgIndex   = Load2DImage("Images/UI/PreGame/btn_idle.png");
    uiBtnSelImgIndex    = Load2DImage("Images/UI/PreGame/btn_selected.png");
    uiUnderlineImgIndex = Load2DImage("Images/UI/PreGame/header_underline.png");

}

void TitleScene::Init()
{
    Scene::Init();
    // タイトル画面でマウス操作ができるようマウスカーソルを表示
    SetMouseDispFlag(TRUE);
    // タイトルBGMの再生（タイトル.mp3をループ再生）
    SoundManager::GetInstance().PlayBGM("title_bgm", 0.85f);
}




TitleScene::~TitleScene()
{
    delete messageWindow;
    messageWindow = nullptr;

    delete settingsWindow;
    settingsWindow = nullptr;
}

void TitleScene::Update()
{
    Scene::Update();

    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080))
    {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }

    bool currMouseLeft = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool mouseClicked = (currMouseLeft && !prevMouseLeft);
    bool mouseMoved = (prevMouseX != -1 && (mouseX != prevMouseX || mouseY != prevMouseY));

    auto& input = InputManager::GetInstance();
    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool upPressed = (currUp && !prevUp) || input.IsPadUpPressed();
    bool downPressed = (currDown && !prevDown) || input.IsPadDownPressed();
    bool confirmPressed = (currEnter && !prevEnter) || input.IsPadConfirmPressed();
    bool cancelPressed = (currEsc && !prevEsc) || input.IsPadCancelPressed();
    waitTimer++;
    if (!currEnter && !currMouseLeft && !input.IsPadConfirmHeld())
    {
        hasReleasedConfirm = true;
    }

    if (state == TitleState::MAIN)
    {
        // --- マウス操作（ホバーで選択項目追従、クリックで即時決定） ---
        const int baseBtnW = 520;
        const int baseBtnH = 117;
        const int startBtnY = 480;
        const int btnSpacing = 120;
        const int btnX = (1920 - baseBtnW) / 2;

        int hoveredBtn = -1;
        for (int i = 0; i < 3; i++)
        {
            int by = startBtnY + i * btnSpacing;
            if (mouseX >= btnX && mouseX <= btnX + baseBtnW &&
                mouseY >= by && mouseY <= by + baseBtnH)
            {
                hoveredBtn = i;
                break;
            }
        }

        if (hoveredBtn != -1)
        {
            if (mouseMoved && cursor != hoveredBtn)
            {
                cursor = hoveredBtn;
            }
            if (mouseClicked && hasReleasedConfirm && waitTimer >= 15)
            {
                cursor = hoveredBtn;
                SoundManager::GetInstance().PlayTapSE();
                if (cursor == 0)
                {
                        SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
                    prevMouseLeft = currMouseLeft;
                    prevMouseX = mouseX;
                    prevMouseY = mouseY;
                    return;
                }
                else if (cursor == 1)
                {
                    state = TitleState::SETTINGS;
                    cursor = 0;
                    prevMouseLeft = currMouseLeft;
                    prevMouseX = mouseX;
                    prevMouseY = mouseY;
                    return;
                }
                else if (cursor == 2)
                {
                    PostQuitMessage(0);
                    return;
                }
            }
        }

        // --- キーボード・ゲームパッド操作 ---
        if (upPressed) cursor--;
        if (downPressed) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        // 長押し・連打による誤進行防止: 一度キーが離されており、かつ最低15フレーム経過していること
        if (hasReleasedConfirm && waitTimer >= 15 && confirmPressed)
        {
            SoundManager::GetInstance().PlayTapSE();
            if (cursor == 0)
            {
                // モードはシングルのみ: 直接キャラクター選択画面へ遷移
                SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
                prevMouseLeft = currMouseLeft;
                prevMouseX = mouseX;
                prevMouseY = mouseY;
                return;
            }
            else if (cursor == 1)
            {
                state = TitleState::SETTINGS;
                cursor = 0;
                prevMouseLeft = currMouseLeft;
                prevMouseX = mouseX;
                prevMouseY = mouseY;
                return;
            }
            else if (cursor == 2)
            {
                PostQuitMessage(0);
                return;
            }
        }
    }
    else if (state == TitleState::SETTINGS)
    {
        const int boxW = 860;
        const int boxX1 = (1920 - boxW) / 2;
        const int boxX2 = boxX1 + boxW;
        const int menuStartY = 315;
        const int menuSpacing = 74;

        // マウス操作（ホバーで選択、クリックで決定）
        for (int i = 0; i < 3; i++)
        {
            int bx1 = boxX1 + 40;
            int bx2 = boxX2 - 40;
            int by1 = menuStartY + i * menuSpacing - 6;
            int by2 = by1 + 52;
            if (mouseX >= bx1 && mouseX <= bx2 && mouseY >= by1 && mouseY <= by2)
            {
                if (mouseMoved && cursor != i) cursor = i;
                if (mouseClicked && hasReleasedConfirm && waitTimer >= 15)
                {
                    cursor = i;
                    SoundManager::GetInstance().PlayTapSE();
                    if (cursor == 0)
                    {
                        GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
                    }
                    else if (cursor == 1)
                    {
                        state = TitleState::CONTROLS;
                        waitTimer = 0;
                        hasReleasedConfirm = false;
                    }
                    else if (cursor == 2)
                    {
                        state = TitleState::MAIN;
                        cursor = 1;
                    }
                }
            }
        }

        if (upPressed) cursor--;
        if (downPressed) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        if (confirmPressed)
        {
            SoundManager::GetInstance().PlayTapSE();
            if (cursor == 0)
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (cursor == 1)
            {
                state = TitleState::CONTROLS;
                waitTimer = 0;
                hasReleasedConfirm = false;
            }
            else if (cursor == 2)
            {
                state = TitleState::MAIN;
                cursor = 1;
            }
        }

        if (cancelPressed)
        {
            SoundManager::GetInstance().PlayTapSE();
            state = TitleState::MAIN;
            cursor = 1;
        }
    }
    else if (state == TitleState::CONTROLS)
    {
        if (hasReleasedConfirm && waitTimer >= 15)
        {
            if (confirmPressed || cancelPressed || mouseClicked)
            {
                SoundManager::GetInstance().PlayTapSE();
                state = TitleState::SETTINGS;
                cursor = 1;
                waitTimer = 0;
                hasReleasedConfirm = false;
            }
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
    prevEsc = currEsc;
    prevMouseLeft = currMouseLeft;
    prevMouseX = mouseX;
    prevMouseY = mouseY;
}

void TitleScene::Draw()
{
    Scene::Draw();

    if (state == TitleState::CONTROLS)
    {
        if (titleBgIndex != -1)
        {
            DrawFullScreen2DImage(titleBgIndex, false);
        }
        else
        {
            DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        }
        UI::ControlsGuide::Draw(1920, 1080);
        return;
    }

    // =========================================================================
    // 1. 背景画像の描画（配列番号から呼び出し）
    // =========================================================================
    if (state == TitleState::SETTINGS)
    {
        int bgH = (settingsBgIndex != -1) ? Get2DImage(settingsBgIndex) : Get2DImage(titleBgIndex);
        HudStyle::DrawTacticalBackdrop(1920, 1080, bgH, 960, 520, 480);
    }
    else if (titleBgIndex != -1)
    {
        // タイトル画面全般の背景画像を配列番号から描画
        DrawFullScreen2DImage(titleBgIndex, false);
    }
    else
    {
        // 画像がない場合のデフォルト背景
        DrawBox(0, 0, 1920, 1080, HudStyle::ColCanvas(), TRUE);
    }

    // =========================================================================
    // 2. タイトルロゴおよび作品名表示 (miuraの過去タイトル: 血痕スプラッター ＋ 作品名「Blind Zone」)
    // =========================================================================
    if (state == TitleState::SETTINGS)
    {
        HudStyle::DrawSectionHeading(1920 / 2, 132, "設定", Get2DImage(uiUnderlineImgIndex), 46);
    }
    else
    {
        // 1. 背面の血痕スプラッターエンブレム (Title02.png)
        int logoHandle = Get2DImage(titleLogoIndex);
        if (logoHandle != -1)
        {
            int logoW = 0, logoH = 0;
            GetGraphSize(logoHandle, &logoW, &logoH);
            int logoX = (1920 - logoW) / 2;
            int logoY = 55;
            DrawGraph(logoX, logoY, logoHandle, TRUE);
        }

        // 2. 作品名ロゴ「Blind Zone」(Title2.png) - 血痕エンブレムの中央に重ねて表示
        int logoHandle2 = Get2DImage(titleLogoIndex2);
        if (logoHandle2 != -1)
        {
            int logoW2 = 0, logoH2 = 0;
            GetGraphSize(logoHandle2, &logoW2, &logoH2);
            int logoX2 = (1920 - logoW2) / 2;
            int logoY2 = (logoHandle != -1) ? 148 : 120;
            DrawGraph(logoX2, logoY2, logoHandle2, TRUE);
        }
        else if (logoHandle == -1)
        {
            // 画像が両方ない場合のテキストタイトルフォールバック
            FontManager::GetInstance().DrawStringCenter(1920 / 2, 200, "=== BLIND ZONE ===", GetColor(255, 255, 255), 42);
        }
    }

    // =========================================================================
    // 3. 各種メニュー描画（文字の視認性向上のため背面に透過プレートを敷く）
    // =========================================================================
    const int menuStartX = 1920 / 2 - 100;
    const int menuStartY = 400;
    const int menuSpacing = 60;

    if (state == TitleState::MAIN)
    {
        const int baseBtnW = 520;
        const int baseBtnH = 117;
        const int startBtnY = 480;
        const int btnSpacing = 120;

        int btnHandles[3] = {
            Get2DImage(btnStartImgIndex),
            Get2DImage(btnSettingsImgIndex),
            Get2DImage(btnExitImgIndex)
        };
        int bannerHandle = Get2DImage(btnBannerImgIndex);

        bool allImagesValid = (btnHandles[0] != -1 && btnHandles[1] != -1 && btnHandles[2] != -1);

        if (allImagesValid)
        {
            for (int i = 0; i < 3; i++)
            {
                bool isSelected = (i == cursor);
                float scale = isSelected ? 1.04f : 0.96f;
                int w = static_cast<int>(baseBtnW * scale);
                int h = static_cast<int>(baseBtnH * scale);
                int bx = (1920 - w) / 2;
                int by = startBtnY + i * btnSpacing - (h - baseBtnH) / 2;

                if (bannerHandle != -1)
                {
                    if (isSelected)
                    {
                        // 選択中: 鮮明な真紅の血痕スプラッターバナー
                        DrawExtendGraph(bx, by, bx + w, by + h, bannerHandle, TRUE);
                    }
                    else
                    {
                        // 非選択時: 薄暗いスモーキーな影バナーでボタン輪郭を確保
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
                        SetDrawBright(60, 60, 70);
                        DrawExtendGraph(bx, by, bx + w, by + h, bannerHandle, TRUE);
                        SetDrawBright(255, 255, 255);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    }
                }

                if (isSelected)
                {
                    // 選択中: 100%輝度でボタン画像を描画
                    DrawExtendGraph(bx, by, bx + w, by + h, btnHandles[i], TRUE);
                }
                else
                {
                    // 非選択時: 透過度75%で描画
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
                    DrawExtendGraph(bx, by, bx + w, by + h, btnHandles[i], TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                }
            }

            // 画面下部に選択中項目の短い案内と控えめなキーバッジを表示
            const char* mainJpTexts[3] = {
                "キャラクターを選んでゲームを始めます",
                "エイム固定や操作説明を確認します",
                "ゲームを終了します"
            };
            FontManager::GetInstance().DrawStringCenterShadow(1920 / 2, 846, mainJpTexts[cursor], HudStyle::ColAmber(), 20);
            int hx = 770;
            hx += HudStyle::DrawKeyHintItem(hx, 892, "W / S", "選ぶ") + 28;
            HudStyle::DrawKeyHintItem(hx, 892, "Enter / クリック", "決定");
        }
        else
        {
            // メニュー用半透明プレート (画像がない場合のフォールバック)
            HudStyle::DrawRoundPanel(menuStartX - 50, menuStartY - 20, 320, 3 * menuSpacing + 30, HudStyle::RadiusPanel, HudStyle::ColBg(), HudStyle::ColBorder(), 180);

            const char* items[] = { "ゲーム開始", "設定", "終了" };
            for (int i = 0; i < 3; i++)
            {
                unsigned int color = (i == cursor) ? HudStyle::ColAmber() : HudStyle::ColText();
                if (i == cursor) FontManager::GetInstance().DrawString(menuStartX - 30, menuStartY + i * menuSpacing, "▶", color, 24);
                FontManager::GetInstance().DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color, 24);
            }
        }
    }
    else if (state == TitleState::SETTINGS)
    {
        const int boxW = 860;
        const int boxX1 = (1920 - boxW) / 2;
        const int boxX2 = boxX1 + boxW;
        const int boxY1 = 260;
        const int boxY2 = 760;

        HudStyle::DrawTexturedPanel(boxX1, boxY1, boxW, boxY2 - boxY1, Get2DImage(uiModalImgIndex), HudStyle::ColBg(), HudStyle::ColBorder(), 245);

        const int sMenuStartY = 315;
        const int sMenuSpacing = 74;

        std::string modeVal = GameSettings::GetInstance().isAimLockHoldMode ? "長押し" : "切り替え";
        std::string items[3] = {
            std::string("エイム固定（Eキー）　：　") + modeVal,
            "操作説明を見る",
            "タイトルへ戻る"
        };

        for (int i = 0; i < 3; i++)
        {
            int iy = sMenuStartY + i * sMenuSpacing;
            bool isSelected = (i == cursor);

            HudStyle::DrawTexturedPanel(boxX1 + 40, iy - 6, boxW - 80, 52,
                                        Get2DImage(isSelected ? uiBtnSelImgIndex : uiBtnIdleImgIndex),
                                        isSelected ? HudStyle::ColCardHover() : HudStyle::ColCard(),
                                        isSelected ? HudStyle::ColAmber() : HudStyle::ColBorderSoft(),
                                        isSelected ? 245 : 210);

            unsigned int color = isSelected ? HudStyle::ColAmber() : HudStyle::ColText();
            if (isSelected)
            {
                FontManager::GetInstance().DrawString(boxX1 + 58, iy + 8, "▶", HudStyle::ColEnemy(), 20);
            }
            FontManager::GetInstance().DrawString(boxX1 + 86, iy + 7, items[i].c_str(), color, 22, isSelected ? 7 : 5);
        }

        // 解説エリア（自然な質感枠）
        int infoY1 = 568;
        int infoY2 = 716;
        HudStyle::DrawTexturedPanel(boxX1 + 40, infoY1, boxW - 80, infoY2 - infoY1, Get2DImage(uiStripImgIndex), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 230);

        int infoTextMaxW = (boxX2 - 64) - (boxX1 + 68);

        if (cursor == 0)
        {
            const char* modeTitle = GameSettings::GetInstance().isAimLockHoldMode ? "長押しモード" : "切り替えモード";
            FontManager::GetInstance().DrawString(boxX1 + 68, infoY1 + 24, modeTitle, HudStyle::ColAmber(), 20, 7);
            const char* descText = GameSettings::GetInstance().isAimLockHoldMode ?
                "Eキーを押している間だけ照準の向きを固定します。" :
                "Eキーを押すたびに照準固定のオン・オフを切り替えます。";
            FontManager::GetInstance().DrawStringWrapped(boxX1 + 68, infoY1 + 62, infoTextMaxW, descText, HudStyle::ColText(), 18, 4);
        }
        else if (cursor == 1)
        {
            FontManager::GetInstance().DrawString(boxX1 + 68, infoY1 + 24, "操作説明", HudStyle::ColAmber(), 20, 7);
            FontManager::GetInstance().DrawStringWrapped(boxX1 + 68, infoY1 + 62, infoTextMaxW, "移動・射撃・スキル・索敵ルールの操作ガイドを開きます。", HudStyle::ColText(), 18, 4);
        }
        else if (cursor == 2)
        {
            FontManager::GetInstance().DrawString(boxX1 + 68, infoY1 + 24, "タイトルへ戻る", HudStyle::ColAmber(), 20, 7);
            FontManager::GetInstance().DrawStringWrapped(boxX1 + 68, infoY1 + 62, infoTextMaxW, "設定を保存してタイトル画面へ戻ります。", HudStyle::ColText(), 18, 4);
        }

        int hx = 680;
        hx += HudStyle::DrawKeyHintItem(hx, 808, "W / S", "選ぶ") + 28;
        hx += HudStyle::DrawKeyHintItem(hx, 808, "Enter / クリック", "決定") + 28;
        HudStyle::DrawKeyHintItem(hx, 808, "Esc", "戻る");
    }
}
