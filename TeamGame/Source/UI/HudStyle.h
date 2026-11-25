#pragma once
#include <DxLib.h>
#include <string>
#include <algorithm>
#include "FontManager.h"

/**
 * @brief ゲーム内HUDの共通デザインシステム・スタイル定義
 * @details スレート・鋼色・琥珀・赤の4系統配色と統一された角丸デザインによるHUD描画ユーティリティ
 */
namespace HudStyle {

    // ========================================================
    // 0. 統一角丸半径プリセット
    // ========================================================
    constexpr int RadiusModal  = 16; ///< 大パネル・モーダルダイアログ
    constexpr int RadiusPanel  = 12; ///< 中パネル・カード・ウィンドウ
    constexpr int RadiusButton = 10; ///< ボタン・バナー・選択項目
    constexpr int RadiusBadge  = 6;  ///< バッジ・キートップ・小ラベル
    constexpr int RadiusGauge  = 4;  ///< HP/MP等のゲージバー

    // ========================================================
    // 1. カラーパレット定義（4系統に限定）
    // ========================================================
    inline unsigned int ColBg()        { return GetColor(10, 13, 18); }    ///< 暗いスレート背景 (10, 13, 18)
    inline unsigned int ColBorder()    { return GetColor(70, 82, 96); }    ///< パネル枠線 (70, 82, 96)
    inline unsigned int ColBorderDim() { return GetColor(45, 54, 64); }    ///< 薄い補助枠線 (45, 54, 64)
    inline unsigned int ColText()      { return GetColor(214, 220, 228); } ///< 本文・主要テキスト (214, 220, 228)
    inline unsigned int ColSubText()   { return GetColor(122, 134, 148); } ///< 補助・ラベル文字 (122, 134, 148)
    inline unsigned int ColAmber()     { return GetColor(232, 170, 60); }  ///< 琥珀アクセント (232, 170, 60)
    inline unsigned int ColAlly()      { return GetColor(168, 188, 208); } ///< 味方・通常ステータス (鋼色 168, 188, 208)
    inline unsigned int ColEnemy()     { return GetColor(214, 66, 58); }   ///< 敵・危険・警告 (赤 214, 66, 58)

    constexpr int DefaultBgAlpha = 210;

    // ========================================================
    // 2. ステータスに応じたカラー判定ヘルパー
    // ========================================================
    /**
     * @brief HP比率に応じたカラー判定（通常=鋼色, 60%以下=琥珀, 30%以下=赤）
     */
    inline unsigned int GetHpColor(float ratio)
    {
        if (ratio <= 0.30f) return ColEnemy(); // 30%以下: 赤
        if (ratio <= 0.60f) return ColAmber(); // 60%以下: 琥珀
        return ColAlly();                      // 通常: 鋼色
    }

    /**
     * @brief 弾数に応じたカラー判定（0=赤, 25%以下=琥珀, 通常=本文色）
     */
    inline unsigned int GetAmmoColor(int current, int maxAmmo)
    {
        if (current <= 0) return ColEnemy();
        if (maxAmmo > 0 && static_cast<float>(current) / maxAmmo <= 0.25f) return ColAmber();
        return ColText();
    }

    // ========================================================
    // 3. 描画コンポーネント関数
    // ========================================================

    /**
     * @brief 矩形サイズに応じて角丸半径を安全にクランプして描画する基本ヘルパー
     */
    inline void DrawRoundBox(int x1, int y1, int x2, int y2, int radius, unsigned int color, int fillFlag)
    {
        int w = x2 - x1;
        int h = y2 - y1;
        if (w <= 0 || h <= 0) return;

        int r = (std::min)(radius, (std::min)(w / 2, h / 2));
        if (r <= 0)
        {
            DrawBox(x1, y1, x2, y2, color, fillFlag);
            return;
        }
        DrawRoundRect(x1, y1, x2, y2, r, r, color, fillFlag);
    }

    /**
     * @brief 角丸矩形の太枠線（同心円状に半径を縮小して重ね描き）
     */
    inline void DrawRoundBorder(int x, int y, int w, int h, int radius, unsigned int borderColor, int thickness = 1)
    {
        for (int i = 0; i < thickness; ++i)
        {
            int r = (std::max)(1, radius - i);
            DrawRoundBox(x + i, y + i, x + w - i, y + h - i, r, borderColor, FALSE);
        }
    }

    /**
     * @brief 半透明背景＋角丸枠線の汎用パネル描画ヘルパー
     */
    inline void DrawRoundPanel(int x, int y, int w, int h, int radius, unsigned int bgColor, unsigned int borderColor, int alpha = 255)
    {
        if (alpha < 255)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            DrawRoundBox(x, y, x + w, y + h, radius, bgColor, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        else
        {
            DrawRoundBox(x, y, x + w, y + h, radius, bgColor, TRUE);
        }
        DrawRoundBox(x, y, x + w, y + h, radius, borderColor, FALSE);
    }

    /**
     * @brief カードやボタン左端に配置する角丸インセットアクセントバー
     */
    inline void DrawAccentPill(int x, int y, int h, unsigned int color, int w = 5)
    {
        if (h <= 0 || w <= 0) return;
        DrawRoundBox(x, y, x + w, y + h, (std::max)(1, w / 2), color, TRUE);
    }

    /**
     * @brief プロ向け角丸パネルの描画（1px枠 + アクセントライン）
     */
    inline void DrawPanel(int x, int y, int w, int h, bool cornerTicks = true, unsigned int cornerColor = 0, int alpha = DefaultBgAlpha)
    {
        if (cornerColor == 0) cornerColor = ColAmber();
        int radius = (h <= 40) ? RadiusButton : RadiusPanel;

        // 1. スレート半透明背景（角丸）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawRoundBox(x, y, x + w, y + h, radius, ColBg(), TRUE);

        // 2. 1px 角丸枠線
        DrawRoundBox(x, y, x + w, y + h, radius, ColBorder(), FALSE);

        // 3. アクセント枠（角丸に沿って強調）
        if (cornerTicks)
        {
            DrawRoundBox(x + 1, y + 1, x + w - 1, y + h - 1, (std::max)(1, radius - 1), cornerColor, FALSE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    /**
     * @brief 細身で洗練された角丸ステータスゲージバーの描画
     */
    inline void DrawGauge(int x, int y, int w, int h, float ratio, unsigned int fillColor, unsigned int emptyColor = 0)
    {
        if (emptyColor == 0) emptyColor = ColBorderDim();
        if (ratio < 0.0f) ratio = 0.0f; else if (ratio > 1.0f) ratio = 1.0f;

        // 下地スロット枠（角丸）
        DrawRoundBox(x, y, x + w, y + h, RadiusGauge, emptyColor, TRUE);

        // 充填部分（角丸）
        int fillW = static_cast<int>(w * ratio);
        if (fillW > 0)
        {
            DrawRoundBox(x, y, x + fillW, y + h, RadiusGauge, fillColor, TRUE);
        }
    }

    /**
     * @brief 中央から外側へ伸びる対抗ゲージの描画（キルスコア用・角丸）
     * @param fromCenterLeft trueなら中央から左へ、falseなら中央から右へ充填
     */
    inline void DrawSplitGauge(int x, int y, int w, int h, float ratio, unsigned int fillColor, bool fromCenterLeft)
    {
        if (ratio < 0.0f) ratio = 0.0f; else if (ratio > 1.0f) ratio = 1.0f;
        // 下地（角丸）
        DrawRoundBox(x, y, x + w, y + h, RadiusGauge, ColBorderDim(), TRUE);

        int fillW = static_cast<int>(w * ratio);
        if (fillW > 0)
        {
            if (fromCenterLeft)
            {
                // 中央（右端）から左方向へ
                DrawRoundBox(x + w - fillW, y, x + w, y + h, RadiusGauge, fillColor, TRUE);
            }
            else
            {
                // 中央（左端）から右方向へ
                DrawRoundBox(x, y, x + fillW, y + h, RadiusGauge, fillColor, TRUE);
            }
        }
    }

    /**
     * @brief 上部中央に縦積みする通知バナーの描画（角丸）
     */
    inline void DrawBanner(int centerX, int y, int w, int h, const std::string& text, unsigned int accentColor, int fontSize = 15)
    {
        int x = centerX - w / 2;

        // 背景ボックス（角丸）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, DefaultBgAlpha);
        DrawRoundBox(x, y, x + w, y + h, RadiusButton, ColBg(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 1px 角丸外枠
        DrawRoundBox(x, y, x + w, y + h, RadiusButton, ColBorder(), FALSE);

        // 左端インセット垂直アクセントピル
        DrawAccentPill(x + 5, y + 4, h - 8, accentColor, 4);

        // テキスト中央描画
        FontManager::GetInstance().DrawStringCenter(centerX + 2, y + (h - fontSize) / 2, text.c_str(), ColText(), fontSize);
    }

} // namespace HudStyle