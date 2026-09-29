#include "DebugManager.h"
#include "DxLib.h"
#include "InputManager.h"
#include <cstdio>

void DebugManager::Update()
{
    // Tabキー または F1キーで暗闇モード / デバッグ表示切り替え
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_TAB) ||
        InputManager::GetInstance().IsKeyPressed(KEY_INPUT_F1))
    {
        ToggleDebugMode();
    }
}

void DebugManager::DrawDebugOverlay(const std::string& stageName, float playerX, float playerY, int enemyCount)
{
    int startX = 1480;
    int startY = 10;
    int boxW = 430;
    int boxH = 150;

    // デバッグ表示の背景ボックス（画面右上に配置して左上UIとの重複を解消）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(startX, startY, startX + boxW, startY + boxH, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(startX, startY, startX + boxW, startY + boxH, GetColor(0, 200, 255), FALSE);

    // デバッグステータス
    unsigned int statusColor = m_isDebugMode ? GetColor(0, 255, 100) : GetColor(255, 180, 0);
    const char* modeStr = m_isDebugMode ? "[DEBUG MODE: ON (全体表示)]" : "[HORROR MODE: ON (暗闇表示)]";
    DrawString(startX + 10, startY + 10, modeStr, statusColor);

    // ステージ＆座標情報
    std::string infoStage = "Stage: " + stageName;
    DrawString(startX + 10, startY + 35, infoStage.c_str(), GetColor(255, 255, 255));

    char posStr[128];
    snprintf(posStr, sizeof(posStr), "Player Pos: (%.1f, %.1f) | Enemies: %d", playerX, playerY, enemyCount);
    DrawString(startX + 10, startY + 55, posStr, GetColor(200, 220, 255));

    // 操作ガイド
    DrawString(startX + 10, startY + 85, "--- DEBUG CONTROLS ---", GetColor(180, 180, 180));
    DrawString(startX + 10, startY + 105, "[TAB / F1] : 暗闇/デバッグ切替", GetColor(255, 255, 255));
    DrawString(startX + 10, startY + 125, "[R] : ステージリセット", GetColor(255, 255, 255));
}