#pragma once

#include "Scene.h"
#include "../Core/PlayerSelection.h"
#include "../UITextWindow.h"
#include <vector>

/**
 * @brief 武器選択画面シーン
 */
class WeaponSelectScene : public Scene {
public:
    WeaponSelectScene();
    ~WeaponSelectScene() override = default;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    WeaponSelectData m_slot1Weapon;               // スロット1 (ハンドガン固定)
    std::vector<WeaponSelectData> m_slot2Options;  // スロット2 選択肢
    int m_selectedSlot2Index = 0;                 // 現在の選択カーソル

    char m_prevKeys[256]{};
    bool IsKeyJustPressed(int keyCode);

    // ========================================================
    // 【2D画像配列キャッシュ & 番号（インデックス）管理】
    // 一度読み込まれた画像は配列にキャッシュされ、番号（インデックス）で呼び出せます。
    // ========================================================
    int m_bgImageIndex = -1;       ///< 背景画像の配列番号
    int m_windowFrameIndex = -1;   ///< UITextWindow枠画像の配列番号

    // UITextWindow による出撃確認枠描画
    UI::UITextWindow m_summaryWindow; ///< 出撃確認用テキストウィンドウ
};
