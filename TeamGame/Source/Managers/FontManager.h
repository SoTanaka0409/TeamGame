#pragma once

#include <DxLib.h>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief DXライブラリ用フォントハンドル一括管理＆キャッシュクラス (シングルトン)
 * 
 * グローバルな SetFontSize によるフォントサイズ汚染を防止し、
 * フォントハンドルの生成・キャッシュ・位置揃え描画（左揃え・中央揃え・右揃え）・破棄を安全かつ高速に行います。
 */
class FontManager {
public:
    /**
     * @brief シングルトンインスタンスの取得
     */
    static FontManager& GetInstance();

    /**
     * @brief 指定した条件のフォントハンドルを取得（なければ自動生成してキャッシュ）
     * @param fontSize フォントサイズ (px)
     * @param thick 太さ (1~9, -1でデフォルト)
     * @param fontName フォント名 (空文字列でデフォルトフォント)
     * @param fontType フォントタイプ (DX_FONTTYPE_ANTIALIASING 等)
     * @return int DXライブラリのフォントハンドル
     */
    int GetFont(int fontSize, int thick = -1, const std::string& fontName = "", int fontType = DX_FONTTYPE_ANTIALIASING);

    /**
     * @brief 指定フォントサイズで文字列を描画 (左揃え)
     */
    void DrawString(int x, int y, const char* str, unsigned int color, int fontSize, int thick = -1, const std::string& fontName = "");

    /**
     * @brief 指定フォントサイズでフォーマット文字列を描画 (左揃え)
     */
    void DrawFormatString(int x, int y, unsigned int color, int fontSize, const char* format, ...);

    /**
     * @brief 指定フォントサイズで文字列を中央揃え描画
     * @param centerX 中央揃えの基準X座標
     * @param y 描画Y座標
     */
    void DrawStringCenter(int centerX, int y, const char* str, unsigned int color, int fontSize, int thick = -1, const std::string& fontName = "");

    /**
     * @brief 指定フォントサイズで文字列を右揃え描画
     * @param rightX 右端の基準X座標
     * @param y 描画Y座標
     */
    void DrawStringRight(int rightX, int y, const char* str, unsigned int color, int fontSize, int thick = -1, const std::string& fontName = "");

    /**
     * @brief 指定したフォントで文字列の描画横幅(px)を取得
     */
    int GetStringWidth(const char* str, int fontSize, int thick = -1, const std::string& fontName = "");

    /**
     * @brief 指定したフォントの高さ(px)を取得
     */
    int GetFontHeight(int fontSize, int thick = -1, const std::string& fontName = "");

    /**
     * @brief キャッシュされている全フォントハンドルを明示的に解放
     */
    void Clear();

private:
    FontManager() = default;
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    std::string MakeKey(int size, int thick, const std::string& name, int type) const;

private:
    std::unordered_map<std::string, int> m_fontMap;
};
