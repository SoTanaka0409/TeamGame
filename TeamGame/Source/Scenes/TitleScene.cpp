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
}
TitleScene::~TitleScene()
{
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
    DrawString(1920 / 2 - 150, 200, "=== TEAM GAME ===", GetColor(255, 255, 255));

    const int menuStartX = 1920 / 2 - 100;
    const int menuStartY = 400;
    const int menuSpacing = 60;

    if (state == TitleState::MAIN)
    {
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

    if (statusMessageTimer > 0 && !statusMessage.empty())
    {
        DrawBox(1920 / 2 - 350, 1080 - 100, 1920 / 2 + 350, 1080 - 50, GetColor(160, 30, 30), TRUE);
        DrawBox(1920 / 2 - 350, 1080 - 100, 1920 / 2 + 350, 1080 - 50, GetColor(255, 150, 150), FALSE);
        DrawString(1920 / 2 - 330, 1080 - 80, statusMessage.c_str(), GetColor(255, 255, 255));
    }
}
