#include "TitleScene.h"
#include "DxLib.h"
#include "GameScene.h"
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
    // 万が一待機中に破棄された場合の安全策
    if (state == TitleState::WAITING && !NetworkManager::GetInstance().IsConnected()) {
        NetworkManager::GetInstance().Disconnect();
    }
}

void TitleScene::Update()
{
    Scene::Update(); // Update objects in scene

    bool currUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool currDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool currEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_SPACE) != 0 || (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool currEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);

    waitTimer++;

    if (state == TitleState::MAIN)
    {
        if (currUp && !prevUp) cursor--;
        if (currDown && !prevDown) cursor++;
        if (cursor < 0) cursor = 4;
        if (cursor > 4) cursor = 0;

        if (currEnter && !prevEnter)
        {
            if (cursor == 0) // Single Player
            {
                NetworkManager::GetInstance().Disconnect(); // Ensure disconnected
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
            }
            else if (cursor == 1) // Host Game
            {
                if (NetworkManager::GetInstance().Listen(9876))
                {
                    udpHandle = MakeUDPSocket(-1);
                    state = TitleState::WAITING;
                }
            }
            else if (cursor == 2) // Join LAN
            {
                udpHandle = MakeUDPSocket(9877);
                state = TitleState::JOINING_LAN;
                waitTimer = 0;
            }
            else if (cursor == 3) // Settings
            {
                state = TitleState::SETTINGS;
                cursor = 0;
            }
            else if (cursor == 4) // Exit
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
    }
    else if (state == TitleState::WAITING)
    {
        waitTimer++;
        NetworkManager::GetInstance().UpdateConnection();
        
        if (NetworkManager::GetInstance().IsConnected())
        {
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
        }
        
        if (currEsc) // キャンセル
        {
            NetworkManager::GetInstance().Disconnect();
            if (udpHandle != -1) { DeleteUDPSocket(udpHandle); udpHandle = -1; }
            state = TitleState::MAIN;
            cursor = 0;
        }

        // Broadcast presence
        if (udpHandle != -1 && waitTimer % 60 == 0)
        {
            IPDATA ip = {255, 255, 255, 255};
            NetWorkSendUDP(udpHandle, ip, 9877, "HOST", 4);
        }
    }
    else if (state == TitleState::JOINING_LAN)
    {
        if (currEsc)
        {
            if (udpHandle != -1) { DeleteUDPSocket(udpHandle); udpHandle = -1; }
            state = TitleState::MAIN;
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
                    SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>());
                }
            }
        }
    }

    prevUp = currUp;
    prevDown = currDown;
    prevEnter = currEnter;
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
        const char* items[] = { "シングルプレイ (1人で遊ぶ)", "ホストになる (部屋を作る)", "LAN参加 (自動検索)", "設定", "ゲーム終了" };
        for (int i = 0; i < 5; i++)
        {
            unsigned int color = (i == cursor) ? GetColor(255, 255, 0) : GetColor(200, 200, 200);
            if (i == cursor) DrawString(menuStartX - 30, menuStartY + i * menuSpacing, ">", color);
            DrawString(menuStartX, menuStartY + i * menuSpacing, items[i], color);
        }
    }
    else if (state == TitleState::SETTINGS)
    {
        DrawString(menuStartX - 50, menuStartY - 100, "--- 設定 ---", GetColor(255, 255, 255));

        std::string items[] = {
            std::string("視点固定モード (Eキー) : ") + (GameSettings::GetInstance().isAimLockHoldMode ? "長押し (ON)" : "切り替え (OFF)"),
            "戻る"
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
        DrawString(menuStartX - 100, menuStartY, "他のプレイヤーを待っています...", GetColor(150, 255, 150));
        
        IPDATA myip;
        int num = 0;
        if (GetMyIPAddress(&myip, 1, &num) == 0 && num > 0)
        {
            char ipStr[64];
            sprintf_s(ipStr, sizeof(ipStr), "あなたのIPアドレス: %d.%d.%d.%d", myip.d1, myip.d2, myip.d3, myip.d4);
            DrawString(menuStartX - 100, menuStartY + 30, ipStr, GetColor(255, 255, 0));
        }

        if (waitTimer % 60 < 30) {
            DrawString(menuStartX, menuStartY + 60, "...", GetColor(150, 255, 150));
        }
        
        DrawString(menuStartX - 50, menuStartY + 120, "[ESC]キーでキャンセル", GetColor(200, 200, 200));
    }
    else if (state == TitleState::JOINING_LAN)
    {
        DrawString(menuStartX - 150, menuStartY, "LAN内のホストを自動検索しています...", GetColor(150, 255, 150));
        
        if (waitTimer % 60 < 30) {
            DrawString(menuStartX, menuStartY + 60, "...", GetColor(150, 255, 150));
        }
        
        DrawString(menuStartX - 50, menuStartY + 120, "[ESC]キーでキャンセル", GetColor(200, 200, 200));
    }
}
