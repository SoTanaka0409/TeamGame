#include "ControlsGuide.h"
#include "../Managers/FontManager.h"
#include <DxLib.h>
#include <cmath>

namespace UI
{

void ControlsGuide::DrawKeyBadge(int x, int y, int w, int h, const char* text, unsigned int borderColor, unsigned int textColor)
{
    // キーボードのキートップ風の立体感
    DrawBox(x, y, x + w, y + h, GetColor(35, 45, 60), TRUE);
    DrawBox(x + 1, y + 1, x + w - 1, y + h - 1, GetColor(50, 65, 85), TRUE);
    DrawBox(x + 2, y + 2, x + w - 2, y + h - 3, GetColor(25, 32, 45), TRUE);

    // 枠線
    DrawBox(x, y, x + w, y + h, borderColor, FALSE);

    // 文字を中央揃え
    FontManager::GetInstance().DrawStringCenter(x + w / 2, y + (h - 18) / 2, text, textColor, 16);
}

void ControlsGuide::Draw(int screenWidth, int screenHeight)
{
    // 1. 全面暗転オーバーレイ (背景を適度に暗くしてフォーカス)
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
    DrawBox(0, 0, screenWidth, screenHeight, GetColor(5, 8, 15), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. メインパネル枠
    int panelW = 1440;
    int panelH = 820;
    int panelX = (screenWidth - panelW) / 2;
    int panelY = (screenHeight - panelH) / 2;

    // パネル背景
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(18, 24, 38), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ネオン枠線 (多重枠)
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 180, 255), FALSE);
    DrawBox(panelX + 2, panelY + 2, panelX + panelW - 2, panelY + panelH - 2, GetColor(0, 120, 180), FALSE);
    DrawBox(panelX + 4, panelY + 4, panelX + panelW - 4, panelY + panelH - 4, GetColor(30, 50, 80), FALSE);

    // 3. パネルヘッダー
    int headerY = panelY + 24;
    FontManager::GetInstance().DrawStringCenter(screenWidth / 2, headerY, "CONTROLS & HOW TO PLAY (操作説明)", GetColor(255, 220, 0), 32);
    FontManager::GetInstance().DrawStringCenter(screenWidth / 2, headerY + 42, "キーボード ＆ マウス 操作ガイド", GetColor(160, 200, 240), 18);
    DrawLine(panelX + 50, headerY + 74, panelX + panelW - 50, headerY + 74, GetColor(0, 140, 200));

    // 4. 3列のカテゴリカード
    int cardGap = 24;
    int cardW = (panelW - 100 - cardGap * 2) / 3; // 約 430px
    int cardH = 570;
    int cardY = headerY + 95;

    // --- 列1: 移動 & 視点 ---
    int col1X = panelX + 50;
    DrawBox(col1X, cardY, col1X + cardW, cardY + cardH, GetColor(12, 16, 26), TRUE);
    DrawBox(col1X, cardY, col1X + cardW, cardY + cardH, GetColor(50, 80, 120), FALSE);
    DrawBox(col1X, cardY, col1X + cardW, cardY + 44, GetColor(25, 45, 75), TRUE);
    FontManager::GetInstance().DrawStringCenter(col1X + cardW / 2, cardY + 10, "【移動 ＆ 照準】", GetColor(100, 220, 255), 22);

    int rowY = cardY + 60;
    int stepY = 68;

    // WASD / 矢印
    DrawKeyBadge(col1X + 20, rowY, 44, 34, "W", GetColor(0, 200, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 70, rowY, 44, 34, "A", GetColor(0, 200, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 120, rowY, 44, 34, "S", GetColor(0, 200, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 170, rowY, 44, 34, "D", GetColor(0, 200, 255), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col1X + 230, rowY + 6, "移動 (8方向)", GetColor(230, 230, 230), 18);
    FontManager::GetInstance().DrawString(col1X + 230, rowY + 30, "※矢印キーでも可能", GetColor(140, 160, 180), 14);

    rowY += stepY;
    // マウスカーソル
    DrawKeyBadge(col1X + 20, rowY, 180, 36, "マウスカーソル", GetColor(255, 200, 50), GetColor(255, 240, 180));
    FontManager::GetInstance().DrawString(col1X + 220, rowY + 8, "照準 (エイム方向)", GetColor(230, 230, 230), 18);

    rowY += stepY;
    // Eキー
    DrawKeyBadge(col1X + 20, rowY, 56, 36, "E", GetColor(100, 255, 150), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col1X + 90, rowY + 8, "エイム固定 (Aim Lock)", GetColor(230, 230, 230), 18);
    FontManager::GetInstance().DrawString(col1X + 90, rowY + 32, "現在の照準向きを固定したまま移動", GetColor(140, 160, 180), 14);

    rowY += stepY;
    // Shiftキー
    DrawKeyBadge(col1X + 20, rowY, 80, 36, "Shift", GetColor(150, 180, 220), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col1X + 115, rowY + 8, "ダッシュ / 高速移動", GetColor(230, 230, 230), 18);

    // --- 列2: 戦闘 & スキル ---
    int col2X = col1X + cardW + cardGap;
    DrawBox(col2X, cardY, col2X + cardW, cardY + cardH, GetColor(12, 16, 26), TRUE);
    DrawBox(col2X, cardY, col2X + cardW, cardY + cardH, GetColor(120, 50, 50), FALSE);
    DrawBox(col2X, cardY, col2X + cardW, cardY + 44, GetColor(75, 25, 25), TRUE);
    FontManager::GetInstance().DrawStringCenter(col2X + cardW / 2, cardY + 10, "【戦闘 ＆ スキル】", GetColor(255, 120, 100), 22);

    rowY = cardY + 60;
    // 左クリック
    DrawKeyBadge(col2X + 20, rowY, 130, 36, "左クリック", GetColor(255, 80, 80), GetColor(255, 220, 220));
    FontManager::GetInstance().DrawString(col2X + 165, rowY + 8, "射撃 / 武器攻撃", GetColor(230, 230, 230), 18);

    rowY += stepY;
    // 右クリック / Space
    DrawKeyBadge(col2X + 20, rowY, 110, 36, "右クリック", GetColor(255, 215, 0), GetColor(255, 255, 200));
    DrawKeyBadge(col2X + 140, rowY, 80, 36, "Space", GetColor(255, 215, 0), GetColor(255, 255, 200));
    FontManager::GetInstance().DrawString(col2X + 235, rowY + 8, "固有スキル発動", GetColor(255, 230, 100), 18);
    FontManager::GetInstance().DrawString(col2X + 235, rowY + 32, "キャラ固有の必殺技", GetColor(180, 160, 120), 14);

    rowY += stepY;
    // Rキー
    DrawKeyBadge(col2X + 20, rowY, 56, 36, "R", GetColor(255, 140, 50), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col2X + 90, rowY + 8, "リロード (弾薬装填)", GetColor(230, 230, 230), 18);
    FontManager::GetInstance().DrawString(col2X + 90, rowY + 32, "弾切れ前に装填可能", GetColor(140, 160, 180), 14);

    rowY += stepY;
    // Q / ホイール
    DrawKeyBadge(col2X + 20, rowY, 56, 36, "Q", GetColor(200, 150, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col2X + 85, rowY, 110, 36, "ホイール", GetColor(200, 150, 255), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col2X + 210, rowY + 8, "武器切り替え", GetColor(230, 230, 230), 18);

    rowY += stepY;
    // Fキー
    DrawKeyBadge(col2X + 20, rowY, 56, 36, "F", GetColor(255, 255, 100), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col2X + 90, rowY + 8, "フラッシュライト ON / OFF", GetColor(230, 230, 230), 18);

    // --- 列3: システム & ゲームルール ---
    int col3X = col2X + cardW + cardGap;
    DrawBox(col3X, cardY, col3X + cardW, cardY + cardH, GetColor(12, 16, 26), TRUE);
    DrawBox(col3X, cardY, col3X + cardW, cardY + cardH, GetColor(60, 100, 70), FALSE);
    DrawBox(col3X, cardY, col3X + cardW, cardY + 44, GetColor(25, 60, 35), TRUE);
    FontManager::GetInstance().DrawStringCenter(col3X + cardW / 2, cardY + 10, "【システム ＆ ルール】", GetColor(120, 255, 160), 22);

    rowY = cardY + 60;
    // チャット 1 2 3 4
    DrawKeyBadge(col3X + 20, rowY, 40, 34, "1", GetColor(100, 220, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col3X + 66, rowY, 40, 34, "2", GetColor(100, 220, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col3X + 112, rowY, 40, 34, "3", GetColor(100, 220, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col3X + 158, rowY, 40, 34, "4", GetColor(100, 220, 255), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col3X + 215, rowY + 6, "無線チャット (連携)", GetColor(230, 230, 230), 18);

    rowY += stepY;
    // Esc
    DrawKeyBadge(col3X + 20, rowY, 65, 36, "Esc", GetColor(200, 200, 200), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col3X + 100, rowY + 8, "ポーズ / 操作説明メニュー", GetColor(230, 230, 230), 18);
    FontManager::GetInstance().DrawString(col3X + 100, rowY + 32, "ゲーム中いつでも操作説明や設定を開けます", GetColor(140, 160, 180), 14);

    rowY += stepY;
    // ゲームのコツ・ルールボックス
    int hintBoxY = rowY + 10;
    int hintBoxH = cardH - (hintBoxY - cardY) - 20;
    DrawBox(col3X + 15, hintBoxY, col3X + cardW - 15, hintBoxY + hintBoxH, GetColor(18, 30, 25), TRUE);
    DrawBox(col3X + 15, hintBoxY, col3X + cardW - 15, hintBoxY + hintBoxH, GetColor(50, 120, 70), FALSE);

    FontManager::GetInstance().DrawString(col3X + 25, hintBoxY + 14, "【暗闇での立ち回り】", GetColor(255, 220, 0), 18);
    FontManager::GetInstance().DrawString(col3X + 25, hintBoxY + 44, "・敵の姿は暗闇では見えない！", GetColor(220, 230, 225), 15);
    FontManager::GetInstance().DrawString(col3X + 25, hintBoxY + 68, "・音の方向に波紋マーカー(HUD)が出現！", GetColor(255, 210, 100), 15);
    FontManager::GetInstance().DrawString(col3X + 25, hintBoxY + 92, "・無音環境でも銃声やソナー波で索敵可能！", GetColor(180, 240, 255), 14);
    FontManager::GetInstance().DrawString(col3X + 25, hintBoxY + 118, "・敵チーム10体撃破で勝利！", GetColor(100, 255, 180), 16);

    // 5. フッター案内
    int footerY = panelY + panelH - 45;
    DrawBox(panelX + 400, footerY - 5, panelX + panelW - 400, footerY + 32, GetColor(25, 40, 60), TRUE);
    DrawBox(panelX + 400, footerY - 5, panelX + panelW - 400, footerY + 32, GetColor(0, 180, 255), FALSE);
    FontManager::GetInstance().DrawStringCenter(screenWidth / 2, footerY, "[ ESC / Enter / Space / クリック ] で戻る", GetColor(255, 240, 120), 18);
}

void ControlsGuide::DrawMiniHud(int screenWidth, int screenHeight)
{
    // ゲーム画面右下に常時表示する半透明ミニガイドバー
    int barW = 860;
    int barH = 34;
    int barX = screenWidth - barW - 15;
    int barY = screenHeight - barH - 12;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    DrawBox(barX, barY, barX + barW, barY + barH, GetColor(10, 15, 25), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(barX, barY, barX + barW, barY + barH, GetColor(0, 140, 200), FALSE);

    // キーアイコンと説明
    int curX = barX + 16;
    int textY = barY + 7;

    auto drawMiniBadge = [&](const char* badge, const char* label, unsigned int badgeCol) {
        int bW = 10 + GetDrawStringWidth(badge, -1);
        DrawBox(curX, barY + 5, curX + bW, barY + barH - 5, GetColor(25, 35, 50), TRUE);
        DrawBox(curX, barY + 5, curX + bW, barY + barH - 5, badgeCol, FALSE);
        FontManager::GetInstance().DrawStringCenter(curX + bW / 2, barY + 7, badge, GetColor(255, 255, 255), 14);
        curX += bW + 6;

        FontManager::GetInstance().DrawString(curX, textY, label, GetColor(200, 220, 240), 15);
        curX += GetDrawStringWidth(label, -1) + 16;
    };

    drawMiniBadge("0 / H", "操作説明", GetColor(255, 215, 0));
    drawMiniBadge("WASD", "移動", GetColor(0, 200, 255));
    drawMiniBadge("左クリック", "射撃", GetColor(255, 80, 80));
    drawMiniBadge("Space", "スキル", GetColor(255, 215, 0));
    drawMiniBadge("R", "装填", GetColor(255, 140, 50));
    drawMiniBadge("E", "エイム固定", GetColor(100, 255, 150));
    drawMiniBadge("Esc", "メニュー", GetColor(180, 180, 180));
}

} // namespace UI
