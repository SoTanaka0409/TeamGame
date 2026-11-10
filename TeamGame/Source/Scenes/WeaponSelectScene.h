#pragma once

#include "Scene.h"
#include "../Core/PlayerSelection.h"
#include "../UI/UITextWindow.h"
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

    enum class SelectState {
        SELECTING,      ///< 武器選択中
        CONFIRMING      ///< 出撃確認モーダル表示中
    };

private:
    WeaponSelectData m_slot1Weapon;               // スロット1 (ハンドガン固定)
    std::vector<WeaponSelectData> m_slot2Options;  // スロット2 選択肢
    int m_selectedSlot2Index = 0;                 // 現在の選択カーソル
    SelectState m_state = SelectState::SELECTING;
    int m_confirmCursor = 0;                     ///< 0: 出撃開始 (START), 1: 装備を変更 (CANCEL)

    char m_prevKeys[256]{};
    bool m_hasReleasedConfirm = false; ///< シーン開始後に決定キーが一度離されたかのフラグ
    int m_prevMouseX = -1;
    int m_prevMouseY = -1;
    bool m_prevMouseLeft = false;

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
