#pragma once

namespace UI
{
/**
 * @brief ゲームの操作説明および視覚的UIガイドを描画するクラス
 */
class ControlsGuide
{
public:
    /**
     * @brief 操作説明モーダルウィンドウ（視覚的キーアイコン・詳細ガイド）を描画
     * @param screenWidth 画面幅 (デフォルト: 1920)
     * @param screenHeight 画面高さ (デフォルト: 1080)
     */
    static void Draw(int screenWidth = 1920, int screenHeight = 1080);

    /**
     * @brief ゲーム画面隅に常時表示するミニ操作案内バーを描画
     * @param screenWidth 画面幅 (デフォルト: 1920)
     * @param screenHeight 画面高さ (デフォルト: 1080)
     */
    static void DrawMiniHud(int screenWidth = 1920, int screenHeight = 1080);

private:
    /**
     * @brief キースタイルのボタン風アイコンを描画する内部補助関数
     */
    static void DrawKeyBadge(int x, int y, int w, int h, const char* text, unsigned int borderColor, unsigned int textColor);
};

} // namespace UI
