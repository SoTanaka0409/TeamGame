#include "ControlsGuide.h"
#include "HudStyle.h"
#include "../Managers/FontManager.h"
#include "../Scenes/Scene.h"
#include <DxLib.h>
#include <cmath>

namespace UI
{

void ControlsGuide::DrawKeyBadge(int x, int y, int w, int h, const char* text, unsigned int borderColor, unsigned int textColor)
{
    // キーボードのキートップ風の立体感
    HudStyle::DrawRoundBox(x, y, x + w, y + h, HudStyle::RadiusBadge, HudStyle::ColCard(), TRUE);
    HudStyle::DrawRoundBox(x + 2, y + 2, x + w - 2, y + h - 3, HudStyle::RadiusBadge - 2, HudStyle::ColCanvas(), TRUE);
    HudStyle::DrawRoundBox(x, y, x + w, y + h, HudStyle::RadiusBadge, borderColor, FALSE);

    // 文字をバッジ内に収まるサイズで中央揃え
    int fitSize = FontManager::GetInstance().GetFittingFontSize(text, w - 12, 17, 13);
    int fontH = FontManager::GetInstance().GetFontHeight(fitSize);
    FontManager::GetInstance().DrawStringCenter(x + w / 2, y + (h - fontH) / 2, text, textColor, fitSize);
}

void ControlsGuide::Draw(int screenWidth, int screenHeight)
{
    // 手描き質感UIパーツを初回のみロード
    static int s_modalIdx = -2;
    static int s_cardIdx = -2;
    static int s_underlineIdx = -2;
    if (s_modalIdx == -2) {
        s_modalIdx     = Scene::Load2DImage("Images/UI/PreGame/panel_modal.png");
        s_cardIdx      = Scene::Load2DImage("Images/UI/PreGame/panel_card_idle.png");
        s_underlineIdx = Scene::Load2DImage("Images/UI/PreGame/header_underline.png");
    }

    // 1. 全面暗転オーバーレイ
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 224);
    DrawBox(0, 0, screenWidth, screenHeight, GetColor(5, 6, 4), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. メインパネル枠（手描き質感）
    int panelW = 1620;
    int panelH = 840;
    int panelX = (screenWidth - panelW) / 2;
    int panelY = (screenHeight - panelH) / 2;

    HudStyle::DrawTexturedPanel(panelX, panelY, panelW, panelH, Scene::Get2DImage(s_modalIdx), HudStyle::ColBg(), HudStyle::ColBorder(), 248);

    // 3. パネルヘッダー（日本語＋血痕ブラシ下線）
    int headerY = panelY + 34;
    HudStyle::DrawSectionHeading(screenWidth / 2, headerY, "操作説明とルール", Scene::Get2DImage(s_underlineIdx), 36);

    // 4. 3列のカテゴリカード
    int cardGap = 28;
    int cardW = (panelW - 110 - cardGap * 2) / 3;
    int cardH = 590;
    int cardY = headerY + 86;

    // --- 列1: 移動・照準 ---
    int col1X = panelX + 55;
    HudStyle::DrawTexturedPanel(col1X, cardY, cardW, cardH, Scene::Get2DImage(s_cardIdx), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 230);
    FontManager::GetInstance().DrawStringCenter(col1X + cardW / 2, cardY + 20, "移動・照準", HudStyle::ColAlly(), 22, 7);
    DrawLine(col1X + 24, cardY + 54, col1X + cardW - 24, cardY + 54, HudStyle::ColBorderDim());

    int rowY = cardY + 74;
    int stepY = 92;

    // WASD
    DrawKeyBadge(col1X + 20, rowY, 42, 36, "W", HudStyle::ColAlly(), HudStyle::ColText());
    DrawKeyBadge(col1X + 68, rowY, 42, 36, "A", HudStyle::ColAlly(), HudStyle::ColText());
    DrawKeyBadge(col1X + 116, rowY, 42, 36, "S", HudStyle::ColAlly(), HudStyle::ColText());
    DrawKeyBadge(col1X + 164, rowY, 42, 36, "D", HudStyle::ColAlly(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col1X + 224, rowY + 3, "移動（8方向）", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col1X + 224, rowY + 30, (col1X + cardW - 20) - (col1X + 224), "矢印キーでも操作可能", HudStyle::ColSubText(), 16, 4);

    rowY += stepY;
    DrawKeyBadge(col1X + 20, rowY, 150, 38, "マウスカーソル", HudStyle::ColAmber(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col1X + 188, rowY + 4, "照準（エイム）", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col1X + 188, rowY + 31, (col1X + cardW - 20) - (col1X + 188), "カーソル方向を向いて狙う", HudStyle::ColSubText(), 16, 4);

    rowY += stepY;
    DrawKeyBadge(col1X + 20, rowY, 52, 38, "E", HudStyle::ColAmber(), HudStyle::ColAmber());
    FontManager::GetInstance().DrawString(col1X + 90, rowY + 4, "エイム固定", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col1X + 90, rowY + 31, (col1X + cardW - 20) - (col1X + 90), "照準向きを固定して移動（設定で変更可）", HudStyle::ColSubText(), 16, 4);

    rowY += stepY + 12;
    DrawKeyBadge(col1X + 20, rowY, 46, 38, "Q", HudStyle::ColAlly(), HudStyle::ColText());
    DrawKeyBadge(col1X + 74, rowY, 96, 38, "ホイール", HudStyle::ColAlly(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col1X + 188, rowY + 4, "武器切り替え", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col1X + 188, rowY + 31, (col1X + cardW - 20) - (col1X + 188), "1 / 2 キーでも切替可能", HudStyle::ColSubText(), 16, 4);

    // --- 列2: 攻撃・スキル ---
    int col2X = col1X + cardW + cardGap;
    HudStyle::DrawTexturedPanel(col2X, cardY, cardW, cardH, Scene::Get2DImage(s_cardIdx), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 230);
    FontManager::GetInstance().DrawStringCenter(col2X + cardW / 2, cardY + 20, "攻撃・スキル", HudStyle::ColEnemy(), 22, 7);
    DrawLine(col2X + 24, cardY + 54, col2X + cardW - 24, cardY + 54, HudStyle::ColBorderDim());

    rowY = cardY + 74;
    int stepY2 = 96;

    DrawKeyBadge(col2X + 20, rowY, 114, 38, "左クリック", HudStyle::ColEnemy(), HudStyle::ColText());
    DrawKeyBadge(col2X + 142, rowY, 46, 38, "Z", HudStyle::ColEnemy(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col2X + 206, rowY + 4, "射撃・攻撃", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col2X + 206, rowY + 31, (col2X + cardW - 20) - (col2X + 206), "長押しで連続射撃", HudStyle::ColSubText(), 16, 4);

    rowY += stepY2;
    DrawKeyBadge(col2X + 20, rowY, 52, 38, "F", HudStyle::ColAmber(), HudStyle::ColAmber());
    FontManager::GetInstance().DrawString(col2X + 90, rowY + 4, "固有スキル発動", HudStyle::ColAmber(), 19);
    FontManager::GetInstance().DrawStringWrapped(col2X + 90, rowY + 31, (col2X + cardW - 20) - (col2X + 90), "キャラクター固有スキルを発動", HudStyle::ColSubText(), 16, 4);

    rowY += stepY2;
    DrawKeyBadge(col2X + 20, rowY, 88, 38, "Space", HudStyle::ColEnemy(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col2X + 124, rowY + 4, "近接ナイフ攻撃", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col2X + 124, rowY + 31, (col2X + cardW - 20) - (col2X + 124), "至近距離の敵を一撃撃破", HudStyle::ColSubText(), 16, 4);

    rowY += stepY2;
    DrawKeyBadge(col2X + 20, rowY, 52, 38, "R", HudStyle::ColAlly(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col2X + 90, rowY + 4, "リロード", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col2X + 90, rowY + 31, (col2X + cardW - 20) - (col2X + 90), "手動で弾薬を装填", HudStyle::ColSubText(), 16, 4);

    rowY += stepY2;
    DrawKeyBadge(col2X + 20, rowY, 114, 38, "右クリック", HudStyle::ColAmber(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col2X + 150, rowY + 4, "ライト切替", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col2X + 150, rowY + 31, (col2X + cardW - 20) - (col2X + 150), "消灯して気配を隠し奇襲", HudStyle::ColSubText(), 16, 4);

    // --- 列3: ルール・立ち回り ---
    int col3X = col2X + cardW + cardGap;
    HudStyle::DrawTexturedPanel(col3X, cardY, cardW, cardH, Scene::Get2DImage(s_cardIdx), HudStyle::ColCard(), HudStyle::ColBorderSoft(), 230);
    FontManager::GetInstance().DrawStringCenter(col3X + cardW / 2, cardY + 20, "ルール・立ち回り", HudStyle::ColAmber(), 22, 7);
    DrawLine(col3X + 24, cardY + 54, col3X + cardW - 24, cardY + 54, HudStyle::ColBorderDim());

    rowY = cardY + 74;
    DrawKeyBadge(col3X + 20, rowY, 68, 38, "Esc", HudStyle::ColBorder(), HudStyle::ColText());
    FontManager::GetInstance().DrawString(col3X + 104, rowY + 4, "ポーズ・設定メニュー", HudStyle::ColText(), 19);
    FontManager::GetInstance().DrawStringWrapped(col3X + 104, rowY + 31, (col3X + cardW - 20) - (col3X + 104), "いつでも中断・設定が可能", HudStyle::ColSubText(), 16, 4);

    rowY += 98;
    int hintBoxY = rowY;
    int hintBoxH = cardH - (hintBoxY - cardY) - 24;
    DrawBox(col3X + 18, hintBoxY, col3X + cardW - 18, hintBoxY + hintBoxH, HudStyle::ColBg(), TRUE);
    DrawBox(col3X + 18, hintBoxY, col3X + cardW - 18, hintBoxY + hintBoxH, HudStyle::ColBorderDim(), FALSE);

    int hintTextX = col3X + 34;
    int hintMaxW = (col3X + cardW - 34) - hintTextX;
    int hy = hintBoxY + 20;

    FontManager::GetInstance().DrawString(hintTextX, hy, "暗闇での立ち回り", HudStyle::ColAmber(), 19, 7);
    hy += 38;

    hy += FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・暗闇や草むらの敵はライト外では見えません。", HudStyle::ColText(), 17, 4) + 14;
    hy += FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・銃声や足音の方向には音波マーカーが出現します。", HudStyle::ColAmber(), 17, 4) + 14;
    hy += FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・音波ピンや薄明イベントを活用して索敵しましょう。", HudStyle::ColAlly(), 17, 4) + 16;
    FontManager::GetInstance().DrawStringWrapped(hintTextX, hy, hintMaxW, "・味方チームで先に10キル達成すると勝利です。", HudStyle::ColText(), 17, 4);

    // 5. フッター案内（控えめなキーバッジ）
    int footerY = panelY + panelH - 54;
    HudStyle::DrawKeyHintItem(screenWidth / 2 - 125, footerY, "Esc / Enter / クリック", "閉じる");
}

void ControlsGuide::DrawMiniHud(int screenWidth, int screenHeight)
{
    int barW = 920;
    int barH = 38;
    int barX = screenWidth - barW - 15;
    int barY = screenHeight - barH - 12;

    HudStyle::DrawRoundPanel(barX, barY, barW, barH, HudStyle::RadiusButton, HudStyle::ColBg(), HudStyle::ColBorder(), 190);

    int curX = barX + 16;

    auto drawMiniBadge = [&](const char* badge, const char* label, unsigned int badgeCol) {
        int bW = 14 + FontManager::GetInstance().GetStringWidth(badge, 15);
        HudStyle::DrawRoundPanel(curX, barY + 5, bW, barH - 10, HudStyle::RadiusBadge, HudStyle::ColCard(), badgeCol);
        FontManager::GetInstance().DrawStringCenter(curX + bW / 2, barY + 10, badge, HudStyle::ColText(), 15);
        curX += bW + 6;

        FontManager::GetInstance().DrawString(curX, barY + 10, label, HudStyle::ColSubText(), 16);
        curX += FontManager::GetInstance().GetStringWidth(label, 16) + 16;
    };

    drawMiniBadge("WASD", "移動", HudStyle::ColAlly());
    drawMiniBadge("左クリック", "射撃", HudStyle::ColEnemy());
    drawMiniBadge("F", "スキル", HudStyle::ColAmber());
    drawMiniBadge("Space", "ナイフ", HudStyle::ColEnemy());
    drawMiniBadge("R", "装填", HudStyle::ColAlly());
    drawMiniBadge("右クリック", "ライト", HudStyle::ColAmber());
    drawMiniBadge("E", "エイム固定", HudStyle::ColAlly());
    drawMiniBadge("Esc", "メニュー", HudStyle::ColBorder());
}

} // namespace UI
