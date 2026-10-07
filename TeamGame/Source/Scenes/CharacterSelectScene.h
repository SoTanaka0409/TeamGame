#pragma once

#include "Scene.h"
#include "../Core/PlayerSelection.h"
#include <vector>

/**
 * @brief キャラクター選択画面シーン
 */
class CharacterSelectScene : public Scene {
public:
    CharacterSelectScene();
    ~CharacterSelectScene() override = default;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    std::vector<CharacterData> m_characters;
    int m_selectedIndex = 0;

    // キー入力判定用
    char m_prevKeys[256]{};
    bool IsKeyJustPressed(int keyCode);
};
