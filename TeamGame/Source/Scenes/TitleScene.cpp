#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "GameSettings.h"
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

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
waitTimer++;
if (!currEnter && !currMouseLeft)
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

        // --- キーボード操作 (W/S/Up/Down/Enter/Space) ---
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        // 長押し・連打による誤進行防止: 一度キーが離されており、かつ最低15フレーム経過していること
        if (hasReleasedConfirm && waitTimer >= 15 && currEnter && !prevEnter)
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
        const int boxW = 580;
        const int boxX1 = (1920 - boxW) / 2;
        const int boxX2 = boxX1 + boxW;
        const int menuStartY = 400;
        const int menuSpacing = 60;

        // マウス操作（ホバーで選択、クリックで決定）
        for (int i = 0; i < 2; i++)
        {
            int bx1 = boxX1;
            int bx2 = boxX2;
            int by1 = menuStartY + i * menuSpacing - 10;
            int by2 = menuStartY + (i + 1) * menuSpacing - 10;
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
                        state = TitleState::MAIN;
                        cursor = 1;
                    }
                }
            }
        }

        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 1;
        if (cursor > 1) cursor = 0;

        if (currEnter && !prevEnter)
        {
            SoundManager::GetInstance().PlayTapSE();
            if (cursor == 0)
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (cursor == 1)
            {
                state = TitleState::MAIN;
                cursor = 1;
            }
        }

        if (currEsc)
        {
            SoundManager::GetInstance().PlayTapSE();
            state = TitleState::MAIN;
            cursor = 1;
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
prevMouseLeft = currMouseLeft;
    prevMouseX = mouseX;
    prevMouseY = mouseY;
}

void TitleScene::Draw()
{
    Scene::Draw();

    // =========================================================================
    // 1. 背景画像の描画（配列番号から呼び出し）
    // =========================================================================
    if (state == TitleState::SETTINGS)
    {
        if (settingsBgIndex != -1)
        {
            // 設定画面用の背景画像を配列番号から描画
            DrawFullScreen2DImage(settingsBgIndex, false);
        }
        else if (titleBgIndex != -1)
        {
            // 設定画像がない場合はタイトル背景に暗幕フィルターを重ねて描画
            DrawFullScreen2DImage(titleBgIndex, false);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
            DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        else
        {
            // 画像がない場合のデフォルト背景
            DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);
        }
    }

    else if (titleBgIndex != -1)
    {
        // タイトル画面全般の背景画像を配列番号から描画
        DrawFullScreen2DImage(titleBgIndex, false);
    }
    else
    {
        // 画像がない場合のデフォルト背景
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
    }

    // =========================================================================
    // 2. タイトルロゴおよび作品名表示 (miuraの過去タイトル: 血痕スプラッター ＋ 作品名「Blind Zone」)
    // =========================================================================
    if (state == TitleState::SETTINGS)
    {
        // 設定画面ヘッダー（中央揃え・580px幅フレーム）
        const int hW = 580;
        const int hX1 = (1920 - hW) / 2;
        const int hX2 = hX1 + hW;
        DrawBox(hX1, 180, hX2, 240, GetColor(20, 30, 45), TRUE);
        DrawBox(hX1, 180, hX2, 240, GetColor(0, 180, 255), FALSE);
        DrawBox(hX1 + 2, 182, hX2 - 2, 238, GetColor(30, 70, 110), FALSE);

        int textW = GetDrawStringWidth("=== 設 定 ===", -1);
        DrawString((1920 - textW) / 2, 200, "=== 設 定 ===", GetColor(255, 255, 255));
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
            DrawString(1920 / 2 - 150, 200, "=== BLIND ZONE ===", GetColor(255, 255, 255));
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

            // 下部に控えめな操作ガイドを表示
            SetFontSize(20);
            int guideW = GetDrawStringWidth("[W/S] キー または マウスで選択  /  [ENTER] または クリックで決定", -1);
            DrawString((1920 - guideW) / 2, 870, "[W/S] キー または マウスで選択  /  [ENTER] または クリックで決定", GetColor(160, 180, 200));
            SetFontSize(32);
        }
        else
        {
            // メニュー用半透明プレート (画像がない場合のフォールバック)
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
            DrawBox(menuStartX - 50, menuStartY - 20, menuStartX + 250, menuStartY + 3 * menuSpacing + 10, GetColor(10, 15, 25), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(menuStartX - 50, menuStartY - 20, menuStartX + 250, menuStartY + 3 * menuSpacing + 10, GetColor(60, 80, 110), FALSE);

            const char* items[] = { "GAME START", "SETTINGS", "EXIT" };
            for (int i = 0; i < 3; i++)
            {
                unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
                if (i == cursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
                DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color);
            }
        }
    }
    else if (state == TitleState::SETTINGS)
    {
        const int boxW = 580;
        const int boxX1 = (1920 - boxW) / 2;
        const int boxX2 = boxX1 + boxW;
        const int boxY1 = menuStartY - 25;
        const int boxY2 = menuStartY + 2 * menuSpacing + 75;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
        DrawBox(boxX1, boxY1, boxX2, boxY2, GetColor(15, 20, 35), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(boxX1, boxY1, boxX2, boxY2, GetColor(0, 160, 240), FALSE);
        DrawBox(boxX1 + 2, boxY1 + 2, boxX2 - 2, boxY2 - 2, GetColor(30, 70, 110), FALSE);

        std::string modeVal = GameSettings::GetInstance().isAimLockHoldMode ? "長押し (Hold)" : "切り替え (Toggle)";
        std::string items[] = {
            std::string("エイム固定 (Eキー) : ") + modeVal,
            "戻る"
        };
        for (int i = 0; i < 2; i++)
        {
            unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(215, 225, 235);
            if (i == cursor)
            {
                DrawString(boxX1 + 35, menuStartY + i * menuSpacing, "▶", GetColor(255, 215, 0));
            }
            DrawString(boxX1 + 65, menuStartY + i * menuSpacing, items[i].c_str(), color);
        }

        // 枠内下部に補足説明を表示（枠内に確実に収まる）
        SetFontSize(18);
        const char* noteText = GameSettings::GetInstance().isAimLockHoldMode ?
            "※ [長押し]: Eキーを押している間だけエイムを固定します" :
            "※ [切り替え]: Eキーを押すごとに固定のON/OFFを切り替えます";
        DrawString(boxX1 + 40, boxY2 - 38, noteText, GetColor(140, 180, 220));
        SetFontSize(32);
    }
}
