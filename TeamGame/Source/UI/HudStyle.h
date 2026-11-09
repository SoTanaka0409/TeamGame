#pragma once
#include <DxLib.h>
#include <string>
#include <algorithm>
#include "FontManager.h"

/**
 * @brief ゲーム内HUDの共通デザインシステム・スタイル定義
 * @details スレート・鋼色・琥珀・赤の4系統配色と角丸・グラデを排したソリッドなプロ向けHUD描画ユーティリティ
 */
namespace HudStyle {

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
     * @brief プロ向けソリッドパネルの描画（1px枠 + 左上/右下のコーナーティック）
     */
    inline void DrawPanel(int x, int y, int w, int h, bool cornerTicks = true, unsigned int cornerColor = 0, int alpha = DefaultBgAlpha)
    {
        if (cornerColor == 0) cornerColor = ColAmber();

        // 1. スレート半透明背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(x, y, x + w, y + h, ColBg(), TRUE);

        // 2. 1px 枠線
        DrawBox(x, y, x + w, y + h, ColBorder(), FALSE);

        // 3. 左上・右下のコーナーティック
        if (cornerTicks)
        {
            int tick = 6;
            // 左上コーナー L字
            DrawLine(x, y, x + tick, y, cornerColor);
            DrawLine(x, y, x, y + tick, cornerColor);
            // 右下コーナー L字
            DrawLine(x + w, y + h, x + w - tick, y + h, cornerColor);
            DrawLine(x + w, y + h, x + w, y + h - tick, cornerColor);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    /**
     * @brief 細身で洗練されたステータスゲージバーの描画
     */
    inline void DrawGauge(int x, int y, int w, int h, float ratio, unsigned int fillColor, unsigned int emptyColor = 0)
    {
        if (emptyColor == 0) emptyColor = ColBorderDim();
        if (ratio < 0.0f) ratio = 0.0f; else if (ratio > 1.0f) ratio = 1.0f;

        // 下地スロット枠
        DrawBox(x, y, x + w, y + h, emptyColor, TRUE);

        // 充填部分
        int fillW = static_cast<int>(w * ratio);
        if (fillW > 0)
        {
            DrawBox(x, y, x + fillW, y + h, fillColor, TRUE);
        }
    }

    /**
     * @brief 中央から外側へ伸びる対抗ゲージの描画（キルスコア用）
     * @param fromCenterLeft trueなら中央から左へ、falseなら中央から右へ充填
     */
    inline void DrawSplitGauge(int x, int y, int w, int h, float ratio, unsigned int fillColor, bool fromCenterLeft)
    {
        if (ratio < 0.0f) ratio = 0.0f; else if (ratio > 1.0f) ratio = 1.0f;
        // 下地
        DrawBox(x, y, x + w, y + h, ColBorderDim(), TRUE);

        int fillW = static_cast<int>(w * ratio);
        if (fillW > 0)
        {
            if (fromCenterLeft)
            {
                // 中央（右端）から左方向へ
                DrawBox(x + w - fillW, y, x + w, y + h, fillColor, TRUE);
            }
            else
            {
                // 中央（左端）から右方向へ
                DrawBox(x, y, x + fillW, y + h, fillColor, TRUE);
            }
        }
    }

    /**
     * @brief 上部中央に縦積みする通知バナーの描画
     */
    inline void DrawBanner(int centerX, int y, int w, int h, const std::string& text, unsigned int accentColor, int fontSize = 15)
    {
        int x = centerX - w / 2;

        // 背景ボックス
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, DefaultBgAlpha);
        DrawBox(x, y, x + w, y + h, ColBg(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 1px 外枠
        DrawBox(x, y, x + w, y + h, ColBorder(), FALSE);

        // 左端垂直アクセントバー (太さ 3px)
        DrawBox(x, y, x + 3, y + h, accentColor, TRUE);

        // テキスト中央描画
        FontManager::GetInstance().DrawStringCenter(centerX + 2, y + (h - fontSize) / 2, text.c_str(), ColText(), fontSize);
    }

} // namespace HudStyle