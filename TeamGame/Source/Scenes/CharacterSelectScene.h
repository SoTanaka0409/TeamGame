#pragma once
#include "Scene.h"
#include <vector>
#include "../Core/PlayerSelection.h"

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
};
