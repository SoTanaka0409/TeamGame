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
    int startX = 1420;
    int startY = 15;
    int boxW = 480;
    int boxH = 175;

    // デバッグ枠背景（右上に配置・枠サイズを拡大して文字のはみ出しを防止）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    DrawBox(startX, startY, startX + boxW, startY + boxH, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(startX, startY, startX + boxW, startY + boxH, GetColor(0, 200, 255), FALSE);

    // モード状態
    unsigned int statusColor = m_isDebugMode ? GetColor(0, 255, 100) : GetColor(255, 180, 0);
    const char* modeStr = m_isDebugMode ? "[DEBUG MODE: ON (全体表示)]" : "[HORROR MODE: ON (暗闇表示)]";
    DrawString(startX + 15, startY + 12, modeStr, statusColor);

    // ステージ情報
    std::string infoStage = "Stage: " + stageName;
    DrawString(startX + 15, startY + 37, infoStage.c_str(), GetColor(255, 255, 255));

    // プレイヤー座標 & 敵数
    char posStr[128];
    snprintf(posStr, sizeof(posStr), "Player Pos: (%.1f, %.1f) | Enemies: %d", playerX, playerY, enemyCount);
    DrawString(startX + 15, startY + 62, posStr, GetColor(200, 220, 255));

    // 操作ガイド
    DrawString(startX + 15, startY + 95, "--- DEBUG CONTROLS ---", GetColor(180, 180, 180));
    DrawString(startX + 15, startY + 118, "[TAB / F1] : 暗闇 / デバッグ切り替え", GetColor(255, 255, 255));
    DrawString(startX + 15, startY + 141, "[R] : ステージリセット", GetColor(255, 255, 255));
}