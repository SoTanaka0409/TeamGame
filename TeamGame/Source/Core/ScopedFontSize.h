#pragma once
#include <DxLib.h>

/**
 * @brief スコープ脱出時に元のフォントサイズへ自動復元するRAIIガードクラス
 * 
 * 使用例:
 * {
 *     ScopedFontSize fontSize(48); // スコープ内のみ48pxに変更
 *     DrawString(100, 100, "タイトル", GetColor(255, 255, 255));
 * } // スコープを抜けると自動的に変更前のフォントサイズに戻る
 */
class ScopedFontSize {
public:
    /**
     * @param newSize このスコープ内で適用したいフォントサイズ
     * @param defaultFallback 万が一元のサイズを取得できなかった場合の復元サイズ (デフォルト 16)
     */
    explicit ScopedFontSize(int newSize, int defaultFallback = 16) {
        int currentSize = GetFontSizeToHandle(-1);
        m_previousSize = (currentSize > 0) ? currentSize : defaultFallback;
        SetFontSize(newSize);
    }

    ~ScopedFontSize() {
        SetFontSize(m_previousSize);
    }

    // コピー・移動を禁止
    ScopedFontSize(const ScopedFontSize&) = delete;
    ScopedFontSize& operator=(const ScopedFontSize&) = delete;
    ScopedFontSize(ScopedFontSize&&) = delete;
    ScopedFontSize& operator=(ScopedFontSize&&) = delete;

private:
    int m_previousSize;
};
