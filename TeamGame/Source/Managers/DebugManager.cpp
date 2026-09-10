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
    // デバッグ情報表示エリアの背景ボックス
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
    DrawBox(10, 10, 420, 160, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(10, 10, 420, 160, GetColor(0, 200, 255), FALSE);

    // デバッグステータス
    unsigned int statusColor = m_isDebugMode ? GetColor(0, 255, 100) : GetColor(255, 180, 0);
    const char* modeStr = m_isDebugMode ? "[DEBUG MODE: ON (明瞭表示)]" : "[HORROR MODE: ON (暗闇表示)]";
    DrawString(20, 20, modeStr, statusColor);

    // ステージ＆座標情報
    std::string infoStage = "Stage: " + stageName;
    DrawString(20, 45, infoStage.c_str(), GetColor(255, 255, 255));

    char posStr[128];
    snprintf(posStr, sizeof(posStr), "Player Pos: (%.1f, %.1f) | Enemies: %d", playerX, playerY, enemyCount);
    DrawString(20, 65, posStr, GetColor(200, 220, 255));

    // 操作コマンドガイド
    DrawString(20, 95, "--- DEBUG CONTROLS ---", GetColor(180, 180, 180));
    DrawString(20, 115, "[TAB / F1] : 暗闇/デバッグ切替", GetColor(255, 255, 255));
    DrawString(20, 135, "[R] : 位置リセット", GetColor(255, 255, 255));
}
