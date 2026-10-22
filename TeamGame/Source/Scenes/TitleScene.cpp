#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameSettings.h"
#include "NetworkManager.h"
#include <string>
#include <random>

namespace {
    constexpr int LAN_PORT = 30000;
    constexpr int LAN_UDP_PORT = 30000;
}

TitleScene::TitleScene()
{
    LoadServerConfig();
}

void TitleScene::LoadServerConfig()
{
    char hostBuf[256] = "localhost";
    GetPrivateProfileStringA("Server", "Host", "localhost", hostBuf, sizeof(hostBuf), ".\\server_config.ini");
    serverHost = hostBuf;
    serverPort = GetPrivateProfileIntA("Server", "Port", 8080, ".\\server_config.ini");
    serverUseSSL = (GetPrivateProfileIntA("Server", "UseSSL", 0, ".\\server_config.ini") != 0);
}

TitleScene::~TitleScene()
{
    if (udpHandle != -1)
    {
        DeleteUDPSocket(udpHandle);
    }
    if ((state == TitleState::WAITING || state == TitleState::WEB_HOST_WAITING) && !NetworkManager::GetInstance().IsConnected()) {
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
        if (cursor < 0) cursor = 5;
        if (cursor > 5) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Solo
            {
                NetworkManager::GetInstance().Disconnect();
                SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
            }
            else if (cursor == 1) // Local PvP
            {
                NetworkManager::GetInstance().Disconnect();
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::LOCAL_PVP));
            }
            else if (cursor == 2) // Local Coop
            {
                NetworkManager::GetInstance().Disconnect();
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::LOCAL_COOP));
            }
            else if (cursor == 3) // Web Online
            {
                state = TitleState::WEB_SELECT;
                cursor = 0;
            }
            else if (cursor == 4) // LAN Play
            {
                state = TitleState::LAN_SELECT;
                cursor = 0;
            }
            else if (cursor == 5) // Back
            {
                state = TitleState::MAIN;
                cursor = 0;
            }
        }
    }
    else if (state == TitleState::WEB_SELECT)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Create Room (Host)
            {
                // 4桁の部屋番号をランダム生成
                int randNum = 1000 + (rand() % 9000);
                sprintf_s(roomCodeBuffer, sizeof(roomCodeBuffer), "%04d", randNum);

                DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(0, 0, 0), TRUE);
                DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(100, 200, 255), FALSE);
                DrawFormatString(1920 / 2 - 200, 1080 / 2 - 10, GetColor(255, 255, 0), "Connecting to server...");
                ScreenFlip();

                if (NetworkManager::GetInstance().ConnectWeb(serverHost, serverPort, roomCodeBuffer, true, serverUseSSL))
                {
                    state = TitleState::WEB_HOST_WAITING;
                    copiedIP = roomCodeBuffer;
                }
                else
                {
                    statusMessage = "Webサーバー接続失敗 (" + serverHost + ")";
                    statusMessageTimer = 300;
                }
            }
            else if (cursor == 1) // Join Room (Client)
            {
                DrawBox(1920 / 2 - 300, 1080 / 2 - 60, 1920 / 2 + 300, 1080 / 2 + 60, GetColor(0, 0, 0), TRUE);
                DrawBox(1920 / 2 - 300, 1080 / 2 - 60, 1920 / 2 + 300, 1080 / 2 + 60, GetColor(200, 200, 200), FALSE);
                DrawString(1920 / 2 - 280, 1080 / 2 - 40, "4桁の部屋番号を入力 (Enterで決定):", GetColor(255, 255, 255));
                ScreenFlip();

                roomCodeBuffer[0] = '\0';
                KeyInputString(1920 / 2 - 280, 1080 / 2 + 10, 15, roomCodeBuffer, FALSE);

                if (strlen(roomCodeBuffer) > 0)
                {
                    DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(0, 0, 0), TRUE);
                    DrawBox(1920 / 2 - 250, 1080 / 2 - 50, 1920 / 2 + 250, 1080 / 2 + 50, GetColor(100, 200, 255), FALSE);
                    DrawFormatString(1920 / 2 - 200, 1080 / 2 - 10, GetColor(255, 255, 0), "Joining room %s ...", roomCodeBuffer);
                    ScreenFlip();

                    if (NetworkManager::GetInstance().ConnectWeb(serverHost, serverPort, roomCodeBuffer, false, serverUseSSL))
                    {
                        SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::NETWORK_CLIENT));
                        return;
                    }
                    else
                    {
                        statusMessage = "接続失敗: 部屋「" + std::string(roomCodeBuffer) + "」に接続できませんでした";
                        statusMessageTimer = 300;
                    }
                }
            }
            else if (cursor == 2) // Back
            {
                state = TitleState::MODE_SELECT;
                cursor = 4;
            }
        }
    }
    else if (state == TitleState::WEB_HOST_WAITING)
    {
        // 相手が部屋に入ってきたかチェック
        if (NetworkManager::GetInstance().HasGuestJoined())
        {
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::NETWORK_HOST));
            return;
        }

        // Cキーで部屋番号をクリップボードにコピー
        if (currC && !prevC)
        {
            SetClipboardText(copiedIP.c_str());
            copyNotifyTimer = 180;
        }

        if (currEsc) // キャンセル
        {
            NetworkManager::GetInstance().Disconnect();
            state = TitleState::WEB_SELECT;
            cursor = 0;
        }
    }
    else if (state == TitleState::LAN_SELECT)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 2;
        if (cursor > 2) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Host LAN
            {
                if (NetworkManager::GetInstance().Listen(LAN_PORT))
                {
                    udpHandle = MakeUDPSocket(-1);
                    state = TitleState::WAITING;
                }
            }
            else if (cursor == 1) // Join LAN
            {
                state = TitleState::JOIN_SELECT;
                cursor = 0;
            }
            else if (cursor == 2) // Back
            {
                state = TitleState::MODE_SELECT;
                cursor = 3;
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
                udpHandle = MakeUDPSocket(LAN_UDP_PORT);
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

                        if (NetworkManager::GetInstance().Connect(targetIP.c_str(), LAN_PORT))
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

                    if (NetworkManager::GetInstance().Connect(ipBuffer, LAN_PORT))
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
            NetWorkSendUDP(udpHandle, ip, LAN_UDP_PORT, "HOST", 4);
            
            // Subnet broadcast fallback
            IPDATA myip[10];
            int num = 0;
            if (GetMyIPAddress(myip, 10, &num) == 0 && num > 0)
            {
                for (int i = 0; i < num; i++)
                {
                    IPDATA bcast = myip[i];
                    bcast.d4 = 255;
                    NetWorkSendUDP(udpHandle, bcast, LAN_UDP_PORT, "HOST", 4);
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
                if (NetworkManager::GetInstance().Connect(ipStr, LAN_PORT))
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
        const char* items[] = { 
            "1. Solo Brawl (ソロ・AIデスマッチ)", 
            "2. Local PvP (画面分割 1P vs 2P対戦)", 
            "3. Local Co-op (画面分割 協力プレイ)", 
            "4. Web Online (インターネット対戦)", 
            "5. LAN Play (ローカルLAN対戦)", 
            "Back" 
        };
        for (int i = 0; i < 6; i++)
        {
            unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
            if (i == cursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
            DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color);
        }

        // 選択中モードの解説表示 (企業審査員への配慮)
        SetFontSize(22);
        if (cursor == 0) {
            DrawString(menuStartX - 100, menuStartY + 6 * menuSpacing + 20, "【ソロ】敵Botと戦いながらキル数を競うモード (1人ですぐ遊べます)", GetColor(180, 220, 255));
        } else if (cursor == 1) {
            DrawString(menuStartX - 100, menuStartY + 6 * menuSpacing + 20, "【3vs3 画面分割対戦】両チームとも人間1人＋Bot2体！(1P: WASD+マウス / 2P: パッドまたは矢印キー)", GetColor(255, 220, 100));
        } else if (cursor == 2) {
            DrawString(menuStartX - 100, menuStartY + 6 * menuSpacing + 20, "【画面分割Co-op】2人で協力して敵Botを撃破するモード", GetColor(180, 255, 180));
        } else if (cursor == 3) {
            DrawString(menuStartX - 100, menuStartY + 6 * menuSpacing + 20, "【オンライン】4桁の部屋番号でマッチングするインターネット対戦", GetColor(200, 200, 200));
        } else if (cursor == 4) {
            DrawString(menuStartX - 100, menuStartY + 6 * menuSpacing + 20, "【LAN対戦】同一ネットワーク内のPC同士で直接接続して対戦", GetColor(200, 200, 200));
        }
        SetFontSize(32);
    }
    else if (state == TitleState::WEB_SELECT)
    {
        const char* items[] = { 
            "Create Room (部屋を作成・ホスト)", 
            "Join Room (部屋番号で参加)", 
            "Back" 
        };
        for (int i = 0; i < 3; i++)
        {
            unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
            if (i == cursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
            DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color);
        }

        // 接続先サーバー情報を小さく表示
        SetFontSize(20);
        DrawFormatString(menuStartX, menuStartY + 3 * menuSpacing + 20, GetColor(150, 150, 150),
            "接続サーバー: %s:%d (%s)", serverHost.c_str(), serverPort, serverUseSSL ? "SSL" : "No-SSL");
        SetFontSize(32);
    }
    else if (state == TitleState::WEB_HOST_WAITING)
    {
        DrawString(1920 / 2 - 200, 360, "=== Web Online 待機中 ===", GetColor(100, 220, 255));

        SetFontSize(48);
        DrawFormatString(1920 / 2 - 220, 440, GetColor(255, 255, 0), "部屋番号:  %s", copiedIP.c_str());
        SetFontSize(32);

        DrawString(1920 / 2 - 250, 530, "[C] キー: 部屋番号をクリップボードにコピー", GetColor(100, 255, 100));
        DrawString(1920 / 2 - 250, 580, "相手に部屋番号を教えてください (ESCでキャンセル)", GetColor(200, 200, 200));

        if (copyNotifyTimer > 0)
        {
            DrawBox(1920 / 2 - 250, 280, 1920 / 2 + 250, 330, GetColor(0, 100, 0), TRUE);
            DrawBox(1920 / 2 - 250, 280, 1920 / 2 + 250, 330, GetColor(100, 255, 100), FALSE);
            DrawFormatString(1920 / 2 - 230, 295, GetColor(255, 255, 255), "★ 部屋番号 %s をコピーしました!", copiedIP.c_str());
        }

        if ((waitTimer / 30) % 2 == 0)
        {
            DrawString(1920 / 2 - 200, 660, "Waiting for Player 2 to join ...", GetColor(255, 255, 100));
        }

        SetFontSize(20);
        DrawFormatString(1920 / 2 - 200, 750, GetColor(140, 140, 140), "サーバー: %s:%d", serverHost.c_str(), serverPort);
        SetFontSize(32);
    }
    else if (state == TitleState::LAN_SELECT)
    {
        const char* items[] = { 
            "Host Game (LAN待機)", 
            "Join Game (LAN参加)", 
            "Back" 
        };
        for (int i = 0; i < 3; i++)
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
            DrawFormatString(1920 / 2 - 200, 1080 / 2 + 40 + (num * 25) + 10, GetColor(180, 220, 255), "Port: %d", LAN_PORT);
            DrawString(1920 / 2 - 200, 1080 / 2 + 40 + (num * 25) + 35, "[C] キー: 自分のIPアドレスをクリップボードにコピー", GetColor(100, 255, 100));
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
