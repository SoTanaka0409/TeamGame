#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameSettings.h"
#include "NetworkManager.h"
#include <string>

TitleScene::TitleScene()
{
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    titleBgIndex       = Load2DImage("Titleu.jpg");
    titleLogoIndex     = Load2DImage("Title02.png");
    titleLogoIndex2     = Load2DImage("Title2.png");
    settingsBgIndex    = Load2DImage("settings_bg.png");
    modeSelectBgIndex  = Load2DImage("mode_select_bg.png");
    windowFrameIndex   = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // UITextWindow によるUIウィンドウの初期化
    messageWindow = new UI::UITextWindow();
    messageWindow->SetMaxTextWidth(700);
    messageWindow->SetPadding(UI::Margin(25, 14, 25, 14));
    messageWindow->SetBoxColor(GetColor(160, 30, 30), GetColor(255, 150, 150), 220);
    if (windowFrameIndex != -1) {
        messageWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }

    settingsWindow = new UI::UITextWindow();
    settingsWindow->SetMaxTextWidth(650);
    settingsWindow->SetPadding(UI::Margin(30, 20, 30, 20));
    settingsWindow->SetBoxColor(GetColor(15, 20, 35), GetColor(0, 160, 240), 180);
    if (windowFrameIndex != -1) {
        settingsWindow->SetBgGraphHandle(Get2DImage(windowFrameIndex));
    }
}

TitleScene::~TitleScene()
{
    // UITextWindow の解放
    delete messageWindow;
    messageWindow = nullptr;

    delete settingsWindow;
    settingsWindow = nullptr;

    if (udpHandle != -1)
    {
        DeleteUDPSocket(udpHandle);
    }
    if (state == TitleState::WAITING && !NetworkManager::GetInstance().IsConnected()) {
        NetworkManager::GetInstance().Disconnect();
    }
}

void TitleScene::Update()
{
    Scene::Update();

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0 || (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool currC = (CheckHitKey(KEY_INPUT_C) != 0);

    waitTimer++;
    if (copyNotifyTimer > 0) copyNotifyTimer--;
    if (statusMessageTimer > 0) statusMessageTimer--;

    if (state == TitleState::MAIN)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) { state = TitleState::MODE_SELECT; cursor = 0; }
            else if (cursor == 1) { state = TitleState::SETTINGS; cursor = 0; }
            else if (cursor == 2) { PostQuitMessage(0); }
        }
    }
    else if (state == TitleState::MODE_SELECT)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 4;
        if (cursor > 4) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Solo
            {
                NetworkManager::GetInstance().Disconnect();
                SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
            }
            else if (cursor == 1) // Local Coop
            {
                NetworkManager::GetInstance().Disconnect();
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::LOCAL_COOP));
            }
            else if (cursor == 2) // Host
            {
                if (NetworkManager::GetInstance().Listen(9876))
                {
                    udpHandle = MakeUDPSocket(-1);
                    state = TitleState::WAITING;
                }
            }
            else if (cursor == 3) // Join
            {
                state = TitleState::JOIN_SELECT;
                cursor = 0;
            }
            else if (cursor == 4) // Back
            {
                state = TitleState::MAIN;
                cursor = 0;
            }
        }
    }
    else if (state == TitleState::JOIN_SELECT)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 3;
        if (cursor > 3) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Join LAN Auto
            {
                udpHandle = MakeUDPSocket(9877);
                state = TitleState::JOINING_LAN;
                waitTimer = 0;
            }
            else if (cursor == 1) // Paste IP & Connect (クリップボードから接続)
            {
                char clipBuf[256] = { 0 };
                if (GetClipboardText(clipBuf) > 0)
                {
                    std::string targetIP = clipBuf;
                    size_t first = targetIP.find_first_not_of(" \t\r\n");
                    size_t last = targetIP.find_last_not_of(" \t\r\n");
                    if (first != std::string::npos && last != std::string::npos)
                    {
                        targetIP = targetIP.substr(first, last - first + 1);
                    }
                    else
                    {
                        targetIP = "";
                    }

                    if (!targetIP.empty())
                    {
                        DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(0, 0, 0), TRUE);
                        DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(100, 200, 255), FALSE);
                        DrawFormatString(1920 / 2 - 220, 1080 / 2 - 10, GetColor(255, 255, 0), "Connecting to %s ...", targetIP.c_str());
                        ScreenFlip();

                        if (NetworkManager::GetInstance().Connect(targetIP.c_str(), 9876))
                        {
                            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::NETWORK_CLIENT));
                            return;
                        }
                        else
                        {
                            statusMessage = "接続失敗: " + targetIP + " に接続できませんでした";
                            statusMessageTimer = 300;
                        }
                    }
                    else
                    {
                        statusMessage = "クリップボードが空です";
                        statusMessageTimer = 180;
                    }
                }
                else
                {
                    statusMessage = "クリップボードにテキストがありません";
                    statusMessageTimer = 180;
                }
            }
            else if (cursor == 2) // Join LAN Manual
            {
                DrawBox(1920 / 2 - 300, 1080 / 2 - 60, 1920 / 2 + 300, 1080 / 2 + 60, GetColor(0, 0, 0), TRUE);
                DrawBox(1920 / 2 - 300, 1080 / 2 - 60, 1920 / 2 + 300, 1080 / 2 + 60, GetColor(200, 200, 200), FALSE);
                DrawString(1920 / 2 - 280, 1080 / 2 - 40, "IP Address (Ctrl+V で貼付け可能):", GetColor(255, 255, 255));
                ScreenFlip();
                
                KeyInputString(1920 / 2 - 280, 1080 / 2 + 10, 63, ipBuffer, FALSE);

                if (strlen(ipBuffer) > 0)
                {
                    DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(0, 0, 0), TRUE);
                    DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(100, 200, 255), FALSE);
                    DrawFormatString(1920 / 2 - 220, 1080 / 2 - 10, GetColor(255, 255, 0), "Connecting to %s ...", ipBuffer);
                    ScreenFlip();

                    if (NetworkManager::GetInstance().Connect(ipBuffer, 9876))
                    {
                        SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::NETWORK_CLIENT));
                        return;
                    }
                    else
                    {
                        statusMessage = std::string("接続失敗: ") + ipBuffer + " に接続できませんでした";
                        statusMessageTimer = 300;
                    }
                }
            }
            else if (cursor == 3) // Back
            {
                state = TitleState::MODE_SELECT;
                cursor = 3;
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
    }
    else if (state == TitleState::WAITING)
    {
        waitTimer++;
        NetworkManager::GetInstance().UpdateConnection();
        
        if (NetworkManager::GetInstance().IsConnected())
        {
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::NETWORK_HOST));
            return;
        }

        // Cキーで自分のIPアドレスをクリップボードにコピー
        if (currC && !prevC)
        {
            IPDATA myip[10];
            int num = 0;
            if (GetMyIPAddress(myip, 10, &num) == 0 && num > 0)
            {
                char ipStr[64];
                sprintf_s(ipStr, sizeof(ipStr), "%d.%d.%d.%d", myip[0].d1, myip[0].d2, myip[0].d3, myip[0].d4);
                SetClipboardText(ipStr);
                copiedIP = ipStr;
                copyNotifyTimer = 180;
            }
        }
        
        if (currEsc) // キャンセル
        {
            NetworkManager::GetInstance().Disconnect();
            if (udpHandle != -1) { DeleteUDPSocket(udpHandle); udpHandle = -1; }
            state = TitleState::MODE_SELECT;
            cursor = 2;
        }

        // Broadcast presence
        if (udpHandle != -1 && waitTimer % 60 == 0)
        {
            // Global broadcast
            IPDATA ip = {255, 255, 255, 255};
            NetWorkSendUDP(udpHandle, ip, 9877, "HOST", 4);
            
            // Subnet broadcast fallback
            IPDATA myip[10];
            int num = 0;
            if (GetMyIPAddress(myip, 10, &num) == 0 && num > 0)
            {
                for (int i = 0; i < num; i++)
                {
                    IPDATA bcast = myip[i];
                    bcast.d4 = 255;
                    NetWorkSendUDP(udpHandle, bcast, 9877, "HOST", 4);
                }
            }
        }
    }
    else if (state == TitleState::JOINING_LAN)
    {
        if (currEsc)
        {
            if (udpHandle != -1) { DeleteUDPSocket(udpHandle); udpHandle = -1; }
            state = TitleState::JOIN_SELECT;
            cursor = 0;
        }
        
        // Listen for broadcast
        if (udpHandle != -1)
        {
            IPDATA hostIP;
            int hostPort;
            char buf[16];
            if (NetWorkRecvUDP(udpHandle, &hostIP, &hostPort, buf, sizeof(buf), FALSE) > 0)
            {
                char ipStr[64];
                sprintf_s(ipStr, sizeof(ipStr), "%d.%d.%d.%d", hostIP.d1, hostIP.d2, hostIP.d3, hostIP.d4);
                if (NetworkManager::GetInstance().Connect(ipStr, 9876))
                {
                    DeleteUDPSocket(udpHandle);
                    udpHandle = -1;
                    SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::NETWORK_CLIENT));
                    return;
                }
            }
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
    prevC = currC;
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
    else if (state == TitleState::MODE_SELECT && modeSelectBgIndex != -1)
    {
        // モード選択画面専用の背景画像を配列番号から描画
        DrawFullScreen2DImage(modeSelectBgIndex, false);
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
    // 2. タイトルロゴまたはタイトルの文字描画
    // =========================================================================
    int logoHandle = Get2DImage(titleLogoIndex);
    if (state == TitleState::SETTINGS)
    {
        // 設定画面ヘッダー
        DrawBox(1920 / 2 - 250, 180, 1920 / 2 + 250, 240, GetColor(20, 30, 45), TRUE);
        DrawBox(1920 / 2 - 250, 180, 1920 / 2 + 250, 240, GetColor(0, 180, 255), FALSE);
        DrawString(1920 / 2 - 90, 200, "=== SETTINGS ===", GetColor(255, 255, 255));
    }
    else if (logoHandle != -1)
    {
        // タイトルロゴ画像が存在する場合、中央揃えで描画
        int logoW = 0, logoH = 0;
        GetGraphSize(logoHandle, &logoW, &logoH);
        int logoX = (1920 - logoW) / 2;
        int logoY = 150;
        DrawGraph(logoX, logoY, logoHandle, TRUE);
    }
    else
    {
        // ロゴ画像がない場合は従来のテキストタイトルを描画
        DrawString(1920 / 2 - 150, 200, "=== TEAM GAME ===", GetColor(255, 255, 255));
    }

    // =========================================================================
    // 2. タイトルロゴまたはタイトルの文字描画
    // =========================================================================
    int logoHandle2 = Get2DImage(titleLogoIndex2);
    if (state == TitleState::SETTINGS)
    {
        // 設定画面ヘッダー
        DrawBox(1920 / 2 - 250, 180, 1920 / 2 + 250, 240, GetColor(20, 30, 45), TRUE);
        DrawBox(1920 / 2 - 250, 180, 1920 / 2 + 250, 240, GetColor(0, 180, 255), FALSE);
        DrawString(1920 / 2 - 90, 200, "=== SETTINGS ===", GetColor(255, 255, 255));
    }
    else if (logoHandle2 != -1)
    {
        // タイトルロゴ画像が存在する場合、中央揃えで描画
        int logoW = 0, logoH = 0;
        GetGraphSize(logoHandle2, &logoW, &logoH);
        int logoX = (1920 - logoW) / 2;
        int logoY = 150+50;
        DrawGraph(logoX, logoY, logoHandle2, TRUE);
    }

    // =========================================================================
    // 3. 各種メニュー描画（文字の視認性向上のため背面に透過プレートを敷く）
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
    else if (state == TitleState::MODE_SELECT)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        DrawBox(menuStartX - 50, menuStartY - 20, menuStartX + 360, menuStartY + 5 * menuSpacing + 10, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(menuStartX - 50, menuStartY - 20, menuStartX + 360, menuStartY + 5 * menuSpacing + 10, GetColor(60, 80, 110), FALSE);

        const char* items[] = { "1P (Solo)", "2P (Same PC - Split Screen)", "Host Game (Network)", "Join Game (Network)", "Back" };
        for (int i = 0; i < 5; i++)
        {
            unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
            if (i == cursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
            DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color);
        }
    }
    else if (state == TitleState::JOIN_SELECT)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        DrawBox(menuStartX - 50, menuStartY - 20, menuStartX + 420, menuStartY + 4 * menuSpacing + 10, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(menuStartX - 50, menuStartY - 20, menuStartX + 420, menuStartY + 4 * menuSpacing + 10, GetColor(60, 80, 110), FALSE);

        const char* items[] = { 
            "Join LAN (Auto Search)", 
            "Paste IP & Connect (クリップボードから接続)", 
            "Enter IP Manually (手動入力)", 
            "Back" 
        };
        for (int i = 0; i < 4; i++)
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
    else if (state == TitleState::WAITING)
    {
        DrawString(1920 / 2 - 200, 1080 / 2, "Waiting for Player 2 to join...", GetColor(255, 255, 0));
        
        IPDATA myip[10];
        int num = 0;
        if (GetMyIPAddress(myip, 10, &num) == 0 && num > 0)
        {
            for (int i = 0; i < num; i++)
            {
                char ipStr[64];
                sprintf_s(ipStr, sizeof(ipStr), "%d.%d.%d.%d", myip[i].d1, myip[i].d2, myip[i].d3, myip[i].d4);
                DrawString(1920 / 2 - 200, 1080 / 2 + 40 + (i * 25), (std::string("Your IP: ") + ipStr).c_str(), GetColor(255, 255, 255));
            }
            DrawString(1920 / 2 - 200, 1080 / 2 + 40 + (num * 25) + 15, "[C] キー: 自分のIPアドレスをクリップボードにコピー", GetColor(100, 255, 100));
        }

        if (copyNotifyTimer > 0)
        {
            DrawBox(1920 / 2 - 250, 1080 / 2 - 90, 1920 / 2 + 250, 1080 / 2 - 50, GetColor(0, 100, 0), TRUE);
            DrawBox(1920 / 2 - 250, 1080 / 2 - 90, 1920 / 2 + 250, 1080 / 2 - 50, GetColor(100, 255, 100), FALSE);
            DrawFormatString(1920 / 2 - 230, 1080 / 2 - 75, GetColor(255, 255, 255), "★ IPアドレスをコピーしました! (%s)", copiedIP.c_str());
        }
        
        if ((waitTimer / 30) % 2 == 0)
        {
            DrawString(1920 / 2 - 150, 1080 / 2 - 40, "Searching for guests...", GetColor(200, 200, 200));
        }
    }
    else if (state == TitleState::JOINING_LAN)
    {
        DrawString(1920 / 2 - 200, 1080 / 2, "Searching for Hosts on LAN...", GetColor(255, 255, 0));
        
        if ((waitTimer / 30) % 2 == 0)
        {
            DrawString(1920 / 2 - 150, 1080 / 2 - 50, "Searching...", GetColor(200, 200, 200));
        }
    }

    // UITextWindow による通知・エラーメッセージ描画
    if (statusMessageTimer > 0 && !statusMessage.empty() && messageWindow)
    {
        messageWindow->SetText(statusMessage);
        int winW = messageWindow->GetWindowWidth();
        int winH = messageWindow->GetWindowHeight();
        messageWindow->Draw((1920 - winW) / 2, 1080 - winH - 40);
    }
}
