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

    // 文字をバッジ内に収まるサイズで中央揃え
    int fitSize = FontManager::GetInstance().GetFittingFontSize(text, w - 12, 18, 14);
    int fontH = FontManager::GetInstance().GetFontHeight(fitSize);
    FontManager::GetInstance().DrawStringCenter(x + w / 2, y + (h - fontH) / 2, text, textColor, fitSize);
}

void ControlsGuide::Draw(int screenWidth, int screenHeight)
{
    // 1. 全面暗転オーバーレイ (背景を適度に暗くしてフォーカス)
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
    DrawBox(0, 0, screenWidth, screenHeight, GetColor(5, 8, 15), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. メインパネル枠 (1660 x 860 で文字が大きくはみ出さない広さを確保)
    int panelW = 1660;
    int panelH = 860;
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
    FontManager::GetInstance().DrawStringCenter(screenWidth / 2, headerY, "CONTROLS & HOW TO PLAY (操作説明)", GetColor(255, 220, 0), 34);
    FontManager::GetInstance().DrawStringCenter(screenWidth / 2, headerY + 44, "キーボード ＆ マウス 操作ガイド", GetColor(160, 200, 240), 20);
    DrawLine(panelX + 50, headerY + 76, panelX + panelW - 50, headerY + 76, GetColor(0, 140, 200));

    // 4. 3列のカテゴリカード
    int cardGap = 24;
    int cardW = (panelW - 100 - cardGap * 2) / 3; // 約 504px
    int cardH = 610;
    int cardY = headerY + 95;

    // --- 列1: 移動 & 照準 ---
    int col1X = panelX + 50;
    DrawBox(col1X, cardY, col1X + cardW, cardY + cardH, GetColor(12, 16, 26), TRUE);
    DrawBox(col1X, cardY, col1X + cardW, cardY + cardH, GetColor(50, 80, 120), FALSE);
    DrawBox(col1X, cardY, col1X + cardW, cardY + 46, GetColor(25, 45, 75), TRUE);
    FontManager::GetInstance().DrawStringCenter(col1X + cardW / 2, cardY + 11, "【移動 ＆ 照準】", GetColor(100, 220, 255), 24);

    int rowY = cardY + 68;
    int stepY = 90;

    // WASD / 矢印
    DrawKeyBadge(col1X + 20, rowY, 44, 36, "W", GetColor(0, 200, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 70, rowY, 44, 36, "A", GetColor(0, 200, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 120, rowY, 44, 36, "S", GetColor(0, 200, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 170, rowY, 44, 36, "D", GetColor(0, 200, 255), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col1X + 232, rowY + 4, "移動 (8方向)", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col1X + 232, rowY + 32, (col1X + cardW - 20) - (col1X + 232), "※矢印キーでも操作可能", GetColor(150, 175, 195), 17, 4);

    rowY += stepY;
    // マウスカーソル
    DrawKeyBadge(col1X + 20, rowY, 170, 38, "マウスカーソル", GetColor(255, 200, 50), GetColor(255, 240, 180));
    FontManager::GetInstance().DrawString(col1X + 206, rowY + 7, "照準 (エイム方向)", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col1X + 206, rowY + 36, (col1X + cardW - 20) - (col1X + 206), "カーソルの方向を向いて狙う", GetColor(150, 175, 195), 17, 4);

    rowY += stepY;
    // Eキー
    DrawKeyBadge(col1X + 20, rowY, 56, 38, "E", GetColor(100, 255, 150), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col1X + 94, rowY + 4, "エイム固定 (Aim Lock)", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col1X + 94, rowY + 32, (col1X + cardW - 20) - (col1X + 94), "現在の照準向きを固定したまま移動 (設定で切替/長押し変更可)", GetColor(150, 175, 195), 17, 4);

    rowY += stepY + 12;
    // Q / ホイール / 1・2
    DrawKeyBadge(col1X + 20, rowY, 48, 38, "Q", GetColor(200, 150, 255), GetColor(255, 255, 255));
    DrawKeyBadge(col1X + 76, rowY, 102, 38, "ホイール", GetColor(200, 150, 255), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col1X + 194, rowY + 4, "武器切り替え", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col1X + 194, rowY + 32, (col1X + cardW - 20) - (col1X + 194), "メイン(1)・サブ(2)キーでも切替可能", GetColor(150, 175, 195), 17, 4);

    // --- 列2: 戦闘 & スキル ---
    int col2X = col1X + cardW + cardGap;
    DrawBox(col2X, cardY, col2X + cardW, cardY + cardH, GetColor(12, 16, 26), TRUE);
    DrawBox(col2X, cardY, col2X + cardW, cardY + cardH, GetColor(120, 50, 50), FALSE);
    DrawBox(col2X, cardY, col2X + cardW, cardY + 46, GetColor(75, 25, 25), TRUE);
    FontManager::GetInstance().DrawStringCenter(col2X + cardW / 2, cardY + 11, "【戦闘 ＆ スキル】", GetColor(255, 120, 100), 24);

    rowY = cardY + 68;
    int stepY2 = 96;

    // 左クリック / Z
    DrawKeyBadge(col2X + 20, rowY, 118, 38, "左クリック", GetColor(255, 80, 80), GetColor(255, 220, 220));
    DrawKeyBadge(col2X + 146, rowY, 48, 38, "Z", GetColor(255, 80, 80), GetColor(255, 220, 220));
    FontManager::GetInstance().DrawString(col2X + 210, rowY + 4, "射撃 / 武器攻撃", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col2X + 210, rowY + 32, (col2X + cardW - 20) - (col2X + 210), "長押しで連続射撃", GetColor(150, 175, 195), 17, 4);

    rowY += stepY2;
    // Fキー (固有スキル発動 - Player.cpp の KEY_INPUT_F に正確に一致)
    DrawKeyBadge(col2X + 20, rowY, 56, 38, "F", GetColor(255, 215, 0), GetColor(255, 255, 200));
    FontManager::GetInstance().DrawString(col2X + 94, rowY + 4, "固有スキル発動", GetColor(255, 230, 100), 20);
    FontManager::GetInstance().DrawStringWrapped(col2X + 94, rowY + 32, (col2X + cardW - 20) - (col2X + 94), "キャラクター固有の特殊スキルを発動", GetColor(190, 175, 130), 17, 4);

    rowY += stepY2;
    // Spaceキー (近接ナイフ攻撃 - Player.cpp の KEY_INPUT_SPACE に正確に一致)
    DrawKeyBadge(col2X + 20, rowY, 92, 38, "Space", GetColor(255, 160, 80), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col2X + 128, rowY + 4, "近接ナイフ攻撃", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col2X + 128, rowY + 32, (col2X + cardW - 20) - (col2X + 128), "至近距離の敵をナイフで一撃撃破", GetColor(150, 175, 195), 17, 4);

    rowY += stepY2;
    // Rキー
    DrawKeyBadge(col2X + 20, rowY, 56, 38, "R", GetColor(255, 140, 50), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col2X + 94, rowY + 4, "リロード (弾薬装填)", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col2X + 94, rowY + 32, (col2X + cardW - 20) - (col2X + 94), "弾切れ前に手動装填が可能", GetColor(150, 175, 195), 17, 4);

    rowY += stepY2;
    // 右クリック (フラッシュライト - Player.cpp の MOUSE_INPUT_RIGHT に正確に一致)
    DrawKeyBadge(col2X + 20, rowY, 120, 38, "右クリック", GetColor(255, 255, 100), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col2X + 156, rowY + 4, "ライト ON / OFF", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col2X + 156, rowY + 32, (col2X + cardW - 20) - (col2X + 156), "視界を照らす（消灯で隠密行動）", GetColor(150, 175, 195), 17, 4);

    // --- 列3: システム & ゲームルール ---
    int col3X = col2X + cardW + cardGap;
    DrawBox(col3X, cardY, col3X + cardW, cardY + cardH, GetColor(12, 16, 26), TRUE);
    DrawBox(col3X, cardY, col3X + cardW, cardY + cardH, GetColor(60, 100, 70), FALSE);
    DrawBox(col3X, cardY, col3X + cardW, cardY + 46, GetColor(25, 60, 35), TRUE);
    FontManager::GetInstance().DrawStringCenter(col3X + cardW / 2, cardY + 11, "【システム ＆ ルール】", GetColor(120, 255, 160), 24);

    rowY = cardY + 68;
    // Esc
    DrawKeyBadge(col3X + 20, rowY, 72, 38, "Esc", GetColor(200, 200, 200), GetColor(255, 255, 255));
    FontManager::GetInstance().DrawString(col3X + 108, rowY + 4, "ポーズ / 設定メニュー", GetColor(235, 235, 235), 20);
    FontManager::GetInstance().DrawStringWrapped(col3X + 108, rowY + 32, (col3X + cardW - 20) - (col3X + 108), "戦闘中いつでもポーズ・設定・操作説明を開けます", GetColor(150, 175, 195), 17, 4);

    rowY += 98;
    // ゲームのコツ・ルールボックス (十分な高さと自動折り返しで絶対に枠はみ出しなし)
    int hintBoxY = rowY;
    int hintBoxH = cardH - (hintBoxY - cardY) - 20;
    DrawBox(col3X + 16, hintBoxY, col3X + cardW - 16, hintBoxY + hintBoxH, GetColor(18, 30, 25), TRUE);
    DrawBox(col3X + 16, hintBoxY, col3X + cardW - 16, hintBoxY + hintBoxH, GetColor(50, 120, 70), FALSE);

    int hintTextX = col3X + 30;
    int hintMaxW = (col3X + cardW - 30) - hintTextX;
    int hy = hintBoxY + 18;

    FontManager::GetInstance().DrawString(hintTextX, hy, "【暗闇での立ち回り】", GetColor(255, 220, 0), 20);
    hy += 38;

    hy += FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・敵の姿は暗闇や草むらでは見えません！", GetColor(225, 235, 230), 18, 4) + 14;
    hy += FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・音の方向に波紋マーカー(HUD)が出現します", GetColor(255, 210, 110), 18, 4) + 14;
    hy += FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・定期的に発生するソナー波や月光を活用して索敵しよう！", GetColor(180, 240, 255), 18, 4) + 16;
    FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・味方チームで先に敵10体撃破で勝利！", GetColor(100, 255, 180), 19, 4);

    // 5. フッター案内
    int footerY = panelY + panelH - 50;
    DrawBox(panelX + 460, footerY - 6, panelX + panelW - 460, footerY + 34, GetColor(25, 40, 60), TRUE);
    DrawBox(panelX + 460, footerY - 6, panelX + panelW - 460, footerY + 34, GetColor(0, 180, 255), FALSE);
    FontManager::GetInstance().DrawStringCenter(screenWidth / 2, footerY + 2, "[ ESC / Enter / Space / クリック ] で戻る", GetColor(255, 240, 120), 20);
}

void ControlsGuide::DrawMiniHud(int screenWidth, int screenHeight)
{
    // ゲーム画面右下に常時表示する半透明ミニガイドバー
    int barW = 920;
    int barH = 38;
    int barX = screenWidth - barW - 15;
    int barY = screenHeight - barH - 12;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    DrawBox(barX, barY, barX + barW, barY + barH, GetColor(10, 15, 25), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(barX, barY, barX + barW, barY + barH, GetColor(0, 140, 200), FALSE);

    int curX = barX + 16;

    auto drawMiniBadge = [&](const char* badge, const char* label, unsigned int badgeCol) {
        int bW = 14 + FontManager::GetInstance().GetStringWidth(badge, 15);
        DrawBox(curX, barY + 5, curX + bW, barY + barH - 5, GetColor(25, 35, 50), TRUE);
        DrawBox(curX, barY + 5, curX + bW, barY + barH - 5, badgeCol, FALSE);
        FontManager::GetInstance().DrawStringCenter(curX + bW / 2, barY + 10, badge, GetColor(255, 255, 255), 15);
        curX += bW + 6;

        FontManager::GetInstance().DrawString(curX, barY + 10, label, GetColor(200, 220, 240), 16);
        curX += FontManager::GetInstance().GetStringWidth(label, 16) + 16;
    };

    drawMiniBadge("WASD", "移動", GetColor(0, 200, 255));
    drawMiniBadge("左クリック", "射撃", GetColor(255, 80, 80));
    drawMiniBadge("F", "スキル", GetColor(255, 215, 0));
    drawMiniBadge("Space", "ナイフ", GetColor(255, 160, 80));
    drawMiniBadge("R", "装填", GetColor(255, 140, 50));
    drawMiniBadge("右クリック", "ライト", GetColor(255, 255, 100));
    drawMiniBadge("E", "エイム固定", GetColor(100, 255, 150));
    drawMiniBadge("Esc", "メニュー", GetColor(180, 180, 180));
}

} // namespace UI
