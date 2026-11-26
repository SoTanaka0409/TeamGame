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
    // 1. カラーパレット定義（暗闇ホラー・手描きテイスト系統）
    // ========================================================
    inline unsigned int ColCanvas()     { return GetColor(10, 11, 9); }    ///< 画面最背面の深炭ベース
    inline unsigned int ColBg()         { return GetColor(18, 19, 16); }   ///< 暗い炭紙背景
    inline unsigned int ColCard()       { return GetColor(24, 25, 21); }   ///< 非選択カード・パネル面
    inline unsigned int ColCardHover()  { return GetColor(35, 34, 27); }   ///< 選択・フォーカスカード面
    inline unsigned int ColBorder()     { return GetColor(102, 98, 84); }  ///< パネル枠線
    inline unsigned int ColBorderSoft() { return GetColor(56, 56, 48); }   ///< 控えめな非選択枠線
    inline unsigned int ColBorderDim()  { return GetColor(38, 39, 33); }   ///< ゲージ下地・薄い補助枠線
    inline unsigned int ColText()       { return GetColor(238, 234, 224); }///< 本文・主要テキスト（生成り白）
    inline unsigned int ColSubText()    { return GetColor(158, 154, 140); }///< 補助・ラベル文字
    inline unsigned int ColAmber()      { return GetColor(232, 188, 92); } ///< 懐中電灯の暖かい光色
    inline unsigned int ColAmberDim()   { return GetColor(142, 110, 48); } ///< 落ち着いた暗琥珀
    inline unsigned int ColAlly()       { return GetColor(156, 178, 134); }///< 味方・通常ステータス (モスグリーン系)
    inline unsigned int ColEnemy()      { return GetColor(198, 46, 42); }  ///< 敵・危険・警告 (血赤)

    constexpr int DefaultBgAlpha = 215;

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

    /**
     * @brief タクティカル・ステルス背景の描画（深黒ベース＋微細な背景テクスチャ＋四隅ビネット＋任意スポットライト）
     */
    inline void DrawTacticalBackdrop(int screenW, int screenH, int bgGraphHandle = -1, int spotX = -1, int spotY = -1, int spotRadius = 380)
    {
        // 1. 深黒ベース
        DrawBox(0, 0, screenW, screenH, ColCanvas(), TRUE);

        // 2. 背景画像がある場合は暗めにトーンを落として質感として敷く
        if (bgGraphHandle != -1)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 92);
            SetDrawBright(158, 154, 140);
            DrawExtendGraph(0, 0, screenW, screenH, bgGraphHandle, FALSE);
            SetDrawBright(255, 255, 255);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 3. 選択中項目を照らす懐中電灯スポットライト（暗闇×ライトの演出）
        if (spotX >= 0 && spotY >= 0 && spotRadius > 0)
        {
            for (int layer = 6; layer >= 1; --layer)
            {
                int r = (spotRadius * layer) / 6;
                int a = (layer <= 2) ? 20 : 10;
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
                DrawCircle(spotX, spotY, r, ColAmber(), TRUE);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 4. 画面上下・左右のソフトビネット（中央に視線を誘導）
        const int vigSteps = 6;
        for (int i = 0; i < vigSteps; ++i)
        {
            int band = (vigSteps - i) * 32;
            int a = 20 + (vigSteps - i) * 15;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
            DrawBox(0, 0, screenW, band, GetColor(5, 6, 4), TRUE);
            DrawBox(0, screenH - band, screenW, screenH, GetColor(5, 6, 4), TRUE);
            DrawBox(0, 0, band, screenH, GetColor(5, 6, 4), TRUE);
            DrawBox(screenW - band, 0, screenW, screenH, GetColor(5, 6, 4), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    /**
     * @brief 手描き質感のPNGパネルを描画する（未読み込み時はフォールバック描画）
     */
    inline void DrawTexturedPanel(int x, int y, int w, int h, int graphHandle, unsigned int fallbackBg, unsigned int fallbackBorder, int alpha = 255)
    {
        if (graphHandle != -1)
        {
            if (alpha < 255)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                DrawExtendGraph(x - 6, y - 6, x + w + 6, y + h + 6, graphHandle, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            else
            {
                DrawExtendGraph(x - 6, y - 6, x + w + 6, y + h + 6, graphHandle, TRUE);
            }
        }
        else
        {
            DrawRoundPanel(x, y, w, h, RadiusPanel, fallbackBg, fallbackBorder, alpha);
        }
    }

    /**
     * @brief 日本語見出し＋手描き血痕アンダーラインを描画
     */
    inline void DrawSectionHeading(int centerX, int y, const char* titleJp, int underlineHandle = -1, int fontSize = 44)
    {
        FontManager::GetInstance().DrawStringCenterShadow(centerX, y, titleJp, ColText(), fontSize, 8);
        if (underlineHandle != -1)
        {
            int textW = FontManager::GetInstance().GetStringWidth(titleJp, fontSize, 8);
            int lineW = (std::max)(260, textW + 120);
            int lx = centerX - lineW / 2;
            int ly = y + fontSize + 6;
            DrawExtendGraph(lx, ly, lx + lineW, ly + 22, underlineHandle, TRUE);
        }
    }

    /**
     * @brief 控えめな操作ガイド用のキーバッジ＋ラベル描画（戻り値: 消費横幅）
     */
    inline int DrawKeyHintItem(int x, int y, const char* keyText, const char* labelText)
    {
        int keyW = FontManager::GetInstance().GetStringWidth(keyText, 14) + 18;
        int badgeH = 26;
        DrawRoundPanel(x, y, keyW, badgeH, 4, ColCard(), ColBorder(), 210);
        FontManager::GetInstance().DrawStringCenter(x + keyW / 2, y + 5, keyText, ColText(), 14);

        int labelX = x + keyW + 8;
        FontManager::GetInstance().DrawString(labelX, y + 5, labelText, ColSubText(), 15);
        int labelW = FontManager::GetInstance().GetStringWidth(labelText, 15);
        return keyW + 8 + labelW;
    }

    /**
     * @brief ラベル＋数値＋細身バーのコンパクトなステータス行描画
     */
    inline void DrawStatBar(int x, int y, int w, const char* label, const char* valueText, float ratio, unsigned int fillColor, int labelSize = 15)
    {
        FontManager::GetInstance().DrawString(x, y, label, ColSubText(), labelSize);
        FontManager::GetInstance().DrawStringRight(x + w, y - 1, valueText, ColText(), labelSize + 1, 6);
        DrawGauge(x, y + labelSize + 6, w, 7, ratio, fillColor, ColBorderDim());
    }

    /**
     * @brief 武器カード用のタクティカル・ベクターシルエット描画
     * @param weaponId 101:Handgun, 201:Shotgun, 202:SniperRifle, 203:SubmachineGun
     */
    inline void DrawWeaponSilhouette(int cx, int cy, int weaponId, unsigned int bodyCol, unsigned int accentCol)
    {
        // 背景の控えめなレティクルリング
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
        DrawCircle(cx, cy, 48, ColBorder(), FALSE);
        DrawLine(cx - 58, cy, cx - 36, cy, ColBorder());
        DrawLine(cx + 36, cy, cx + 58, cy, ColBorder());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        if (weaponId == 101) // Handgun
        {
            // スライド＋バレル
            DrawRoundBox(cx - 46, cy - 18, cx + 48, cy - 1, 3, bodyCol, TRUE);
            // フレーム
            DrawRoundBox(cx - 40, cy - 2, cx + 28, cy + 6, 2, bodyCol, TRUE);
            // グリップ
            DrawQuadrangle(cx - 38, cy + 4, cx - 18, cy + 4, cx - 26, cy + 34, cx - 46, cy + 34, bodyCol, TRUE);
            // トリガーガード＆サイト
            DrawBox(cx - 18, cy + 6, cx + 2, cy + 16, bodyCol, FALSE);
            DrawBox(cx + 40, cy - 22, cx + 45, cy - 18, accentCol, TRUE);
        }
        else if (weaponId == 201) // Shotgun
        {
            // 長銃身＋チューブマガジン
            DrawRoundBox(cx - 25, cy - 14, cx + 88, cy - 5, 2, bodyCol, TRUE);
            DrawRoundBox(cx + 2, cy - 4, cx + 76, cy + 3, 2, accentCol, TRUE);
            // ポンプフォアエンド
            DrawRoundBox(cx + 12, cy - 5, cx + 54, cy + 6, 3, accentCol, TRUE);
            // レシーバー
            DrawRoundBox(cx - 48, cy - 15, cx + 8, cy + 6, 3, bodyCol, TRUE);
            // ストック
            DrawQuadrangle(cx - 46, cy - 10, cx - 46, cy + 4, cx - 94, cy + 18, cx - 96, cy - 2, bodyCol, TRUE);
        }
        else if (weaponId == 202) // Sniper Rifle
        {
            // ロングバレル＋マズルブレーキ
            DrawRoundBox(cx - 8, cy - 10, cx + 102, cy - 4, 2, bodyCol, TRUE);
            DrawRoundBox(cx + 96, cy - 12, cx + 110, cy - 2, 2, accentCol, TRUE);
            // スコープ＋マウント
            DrawRoundBox(cx - 26, cy - 28, cx + 32, cy - 18, 4, accentCol, TRUE);
            DrawBox(cx - 18, cy - 18, cx - 12, cy - 12, bodyCol, TRUE);
            DrawBox(cx + 14, cy - 18, cx + 20, cy - 12, bodyCol, TRUE);
            // レシーバー＋マガジン
            DrawRoundBox(cx - 46, cy - 12, cx + 16, cy + 4, 3, bodyCol, TRUE);
            DrawRoundBox(cx - 12, cy + 4, cx + 4, cy + 20, 2, bodyCol, TRUE);
            // グリップ＆スケルトンストック
            DrawQuadrangle(cx - 42, cy + 3, cx - 28, cy + 3, cx - 34, cy + 24, cx - 48, cy + 24, bodyCol, TRUE);
            DrawRoundBox(cx - 104, cy - 10, cx - 44, cy + 8, 3, bodyCol, TRUE);
            DrawRoundBox(cx - 92, cy - 5, cx - 54, cy + 3, 2, ColCanvas(), TRUE);
        }
        else // Submachine Gun (203)
        {
            // アッパー＆サプレッサー風バレル
            DrawRoundBox(cx - 44, cy - 16, cx + 42, cy + 4, 3, bodyCol, TRUE);
            DrawRoundBox(cx + 42, cy - 12, cx + 74, cy - 2, 3, accentCol, TRUE);
            // コンパクトサイト
            DrawRoundBox(cx - 14, cy - 23, cx + 10, cy - 16, 2, accentCol, TRUE);
            // ロングマガジン
            DrawQuadrangle(cx + 4, cy + 4, cx + 18, cy + 4, cx + 12, cy + 36, cx - 2, cy + 36, accentCol, TRUE);
            // グリップ＆フォールディングストック
            DrawQuadrangle(cx - 38, cy + 4, cx - 22, cy + 4, cx - 28, cy + 28, cx - 44, cy + 28, bodyCol, TRUE);
            DrawRoundBox(cx - 82, cy - 12, cx - 44, cy - 6, 2, bodyCol, TRUE);
            DrawRoundBox(cx - 84, cy - 12, cx - 76, cy + 10, 2, bodyCol, TRUE);
        }
    }

} // namespace HudStyle