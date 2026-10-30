#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameSettings.h"
#include <string>

TitleScene::TitleScene()
{
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // =========================================================================
    titleBgIndex       = Load2DImage("Titleu.jpg");
    if (titleBgIndex == -1) titleBgIndex = Load2DImage("title_bg.png");
    titleLogoIndex     = Load2DImage("Title02.png");
    if (titleLogoIndex == -1) titleLogoIndex = Load2DImage("Title.png");
    titleLogoIndex2    = Load2DImage("Title2.png");
    settingsBgIndex    = Load2DImage("settings_bg.png");
    windowFrameIndex   = Load2DImage("ui_window.png"); // 9スライス用枠画像
}

TitleScene::~TitleScene()
{
    delete settingsWindow;
    settingsWindow = nullptr;
}

void TitleScene::Update()
{
    Scene::Update();

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0 || (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);

    waitTimer++;

    if (state == TitleState::MAIN)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // GAME START (シングルプレイ: キャラクター選択へ)
            {
                SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
            }
            else if (cursor == 1) // SETTINGS
            {
                state = TitleState::SETTINGS;
                cursor = 0;
            }
            else if (cursor == 2) // EXIT
            {
                PostQuitMessage(0);
            }
        }
    }
    else if (state == TitleState::SETTINGS)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 1;
        if (cursor > 1) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Toggle Aim Lock Mode
            {
                GameSettings::GetInstance().isAimLockHoldMode = !GameSettings::GetInstance().isAimLockHoldMode;
            }
            else if (cursor == 1) // Back
            {
                state = TitleState::MAIN;
                cursor = 1;
            }
        }
        else if (currEsc)
        {
            state = TitleState::MAIN;
            cursor = 1;
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
}

void TitleScene::Draw()
{
    Scene::Draw();

    // =========================================================================
    // 1. 背景画像の描画
    // =========================================================================
    if (state == TitleState::SETTINGS)
    {
        if (settingsBgIndex != -1)
        {
            DrawFullScreen2DImage(settingsBgIndex, false);
        }
        else if (titleBgIndex != -1)
        {
            DrawFullScreen2DImage(titleBgIndex, false);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
            DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        else
        {
            DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);
        }
    }
    else if (titleBgIndex != -1)
    {
        DrawFullScreen2DImage(titleBgIndex, false);
    }
    else
    {
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
    }

    // =========================================================================
    // 2. タイトルロゴおよび作品名表示 (血痕スプラッター ＋ 「Blind Zone」)
    // =========================================================================
    if (state == TitleState::SETTINGS)
    {
        DrawBox(1920 / 2 - 250, 180, 1920 / 2 + 250, 240, GetColor(20, 30, 45), TRUE);
        DrawBox(1920 / 2 - 250, 180, 1920 / 2 + 250, 240, GetColor(0, 180, 255), FALSE);
        DrawString(1920 / 2 - 90, 200, "=== SETTINGS ===", GetColor(255, 255, 255));
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

        // 2. 作品名ロゴ「Blind Zone」(Title2.png)
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
            DrawString(1920 / 2 - 150, 200, "=== BLIND ZONE ===", GetColor(255, 255, 255));
        }
    }

    // =========================================================================
    // 3. メニュー描画
    // =========================================================================
    const int menuStartX = 1920 / 2 - 100;
    const int menuStartY = 400;
    const int menuSpacing = 60;

    if (state == TitleState::MAIN)
    {
        // メニュー用半透明プレート
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
    else if (state == TitleState::SETTINGS)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
        DrawBox(menuStartX - 60, menuStartY - 20, menuStartX + 440, menuStartY + 2 * menuSpacing + 20, GetColor(15, 20, 35), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(menuStartX - 60, menuStartY - 20, menuStartX + 440, menuStartY + 2 * menuSpacing + 20, GetColor(0, 160, 240), FALSE);

        std::string items[] = {
            std::string("Aim Lock Mode (E key) : ") + (GameSettings::GetInstance().isAimLockHoldMode ? "Hold" : "Toggle"),
            "Back"
        };
        for (int i = 0; i < 2; i++)
        {
            unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
            if (i == cursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
            DrawString(menuStartX, menuStartY + i * menuSpacing, items[i].c_str(), color);
        }
    }
}
