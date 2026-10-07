#pragma once

#include "Scene.h"
#include "../Core/PlayerSelection.h"
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
};
