#pragma once

#include <DxLib.h>
#include <string>
#include <vector>
#include <algorithm>

namespace UI {

/**
 * @brief 文字コードエンコーディング指定
 */
enum class TextEncoding {
    UTF8,
    SJIS
};

/**
 * @brief マージン・パディング指定用構造体
 */
struct Margin {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    Margin() = default;
    Margin(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    explicit Margin(int all) : left(all), top(all), right(all), bottom(all) {}
};

/**
 * @brief 9-Slicing背景自動追従テキストウィンドウクラス
 */
class UITextWindow {
public:
    UITextWindow();
    ~UITextWindow() = default;

    // --- Setter ---
    void SetText(const std::string& text);
    void SetMaxTextWidth(int maxTextWidth);
    void SetFontHandle(int fontHandle);
    void SetTextColor(unsigned int color);
    void SetBgGraphHandle(int graphHandle);
    void SetPadding(const Margin& padding);
    void SetNineSliceMargin(const Margin& margin);
    void SetLineSpacing(int lineSpacing);
    void SetTextEncoding(TextEncoding encoding);
    void SetDrawTransparentFlag(int transparentFlag);

    // --- Getter ---
    const std::string& GetText() const { return m_rawText; }
    int GetMaxTextWidth() const { return m_maxTextWidth; }
    int GetWindowWidth() const;
    int GetWindowHeight() const;
    int GetTextWidth() const;
    int GetTextHeight() const;
    const std::vector<std::string>& GetWrappedLines() const;

    // --- レイアウト計算・描画 ---
    /**
     * @brief テキストの折り返し・ウィンドウサイズの再計算を行います。
     *        (SetTextやSetMaxTextWidth実行時に自動で呼ばれますが、明示的に呼び出すことも可能です)
     */
    void RecalculateLayout();

    /**
     * @brief 指定した画面座標にUIウィンドウとテキストを描画します。
     * @param x 描画基準X座標 (左上)
     * @param y 描画基準Y座標 (左上)
     */
    void Draw(int x, int y) const;

private:
    // 9スライス描画内部関数
    void Draw9Slice(int x, int y, int width, int height) const;

    // 1文字のバイト長を取得する（UTF-8 / SJIS対応）
    size_t GetNextCharByteLen(const char* p) const;

private:
    std::string m_rawText;
    int m_maxTextWidth = 300;                         // テキストの最大折り返し幅(px)
    int m_fontHandle = -1;                            // フォントハンドル (-1はDXライブラリデフォルト)
    unsigned int m_textColor = GetColor(255, 255, 255); // テキスト色 (デフォルト白)
    int m_bgGraphHandle = -1;                         // 9スライスに使用する背景画像ハンドル
    int m_lineSpacing = 4;                            // 行間(px)
    int m_transFlag = TRUE;                           // 透過描画フラグ (TRUE/FALSE)
    TextEncoding m_encoding = TextEncoding::UTF8;     // 文字コード判定設定

    Margin m_padding{ 16, 16, 16, 16 };               // ウィンドウ枠とテキストの余白
    Margin m_nineSliceMargin{ 16, 16, 16, 16 };       // 9スライスの4隅コーナーサイズ

    // レイアウト計算保持変数
    mutable std::vector<std::string> m_wrappedLines;  // 折り返し後の各行文字列
    mutable int m_calculatedTextWidth = 0;            // 計算されたテキスト最大幅
    mutable int m_calculatedTextHeight = 0;           // 計算されたテキスト全体高さ
    mutable int m_windowWidth = 0;                    // 全体ウィンドウ幅
    mutable int m_windowHeight = 0;                   // 全体ウィンドウ高さ
    mutable bool m_isDirty = true;                    // レイアウト再計算フラグ
};

} // namespace UI
