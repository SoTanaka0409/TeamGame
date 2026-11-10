#pragma once
#include "Scene.h"
#include <vector>
#include "../Core/PlayerSelection.h"
#include "../UI/UITextWindow.h"

/**
 * @brief キャラクター選択画面シーン
 */
class CharacterSelectScene : public Scene {
public:
    CharacterSelectScene();
    virtual ~CharacterSelectScene();

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

    enum class SelectState {
        SELECTING,      ///< キャラクター選択中
        CONFIRMING      ///< 決定確認モーダル表示中
    };

private:
    int m_cursorIndex = 0;
    int m_selectedIndex = 0;
    SelectState m_state = SelectState::SELECTING;
    int m_confirmCursor = 0; ///< 0: 決定 (YES), 1: 選び直す (CANCEL)

    bool m_hasReleasedConfirm = false; ///< シーン開始後に決定キーが一度離されたかのフラグ
    int m_prevMouseX = -1;
    int m_prevMouseY = -1;
    bool m_prevMouseLeft = false;

    std::vector<SelectionCharacterData> m_characters;
    std::vector<int> m_previewHandles;

    // ========================================================
    // 【2D画像配列キャッシュ & 番号（インデックス）管理】
    // 一度読み込まれた画像は配列にキャッシュされ、番号（インデックス）で呼び出せます。
    // ========================================================
    int m_bgImageIndex = -1;       ///< 背景画像の配列番号
    int m_windowFrameIndex = -1;   ///< UITextWindow枠画像の配列番号

    // UITextWindow によるキャラクター説明枠描画
    UI::UITextWindow m_detailWindow; ///< 詳細説明用テキストウィンドウ
};
