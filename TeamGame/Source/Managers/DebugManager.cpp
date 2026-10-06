#include "DebugManager.h"
#include "DxLib.h"
#include "InputManager.h"
#include <cstdio>

void DebugManager::Update()
{
    // 0キー（またはテンキー0, Tab, F1）でデバッグモードのON/OFF切り替え
    if (InputManager::GetInstance().IsKeyPressed(KEY_INPUT_0) ||
        InputManager::GetInstance().IsKeyPressed(KEY_INPUT_NUMPAD0) ||
        InputManager::GetInstance().IsKeyPressed(KEY_INPUT_TAB) ||
        InputManager::GetInstance().IsKeyPressed(KEY_INPUT_F1))
    {
        ToggleDebugMode();
    }
}

void DebugManager::DrawDebugOverlay(const std::string& stageName, float playerX, float playerY, int enemyCount)
{
    // デバッグモードがOFFの場合はオーバーレイ画面を描画しない
    if (!m_isDebugMode) return;

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
    unsigned int statusColor = GetColor(0, 255, 100);
    const char* modeStr = "[DEBUG MODE: ON (全体表示・キー有効)]";
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
    DrawString(startX + 15, startY + 118, "[0 / TAB] : デバッグ表示 ON/OFF", GetColor(255, 255, 255));
    DrawString(startX + 15, startY + 141, "[T] テーマ変更 | [L] マップ変更", GetColor(255, 255, 255));
}