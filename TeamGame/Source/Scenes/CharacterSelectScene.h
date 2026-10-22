#pragma once
#include "Scene.h"
#include <vector>
#include "../Core/PlayerSelection.h"
#include "../UITextWindow.h"

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

private:
    int m_cursorIndex;
    int m_selectedIndex;
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
