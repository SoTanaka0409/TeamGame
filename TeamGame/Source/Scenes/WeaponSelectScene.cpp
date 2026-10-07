#include "WeaponSelectScene.h"
#include "GameScene.h"
#include "../Managers/SceneManager.h"
#include "../Managers/FontManager.h"
#include "../Managers/InputManager.h"
#include <DxLib.h>

WeaponSelectScene::WeaponSelectScene() {
}

void WeaponSelectScene::Init() {
    m_slot1Weapon = PlayerSelectionManager::GetFixedSlot1Weapon();
    m_slot2Options = PlayerSelectionManager::GetAvailableSlot2Weapons();
    m_selectedSlot2Index = 0;
}

void WeaponSelectScene::Update() {
    int count = static_cast<int>(m_slot2Options.size());
    auto& input = InputManager::GetInstance();

    // 左右・WASDでスロット2の武器を選択
    if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A)) {
        m_selectedSlot2Index = (m_selectedSlot2Index - 1 + count) % count;
    }
    if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D)) {
        m_selectedSlot2Index = (m_selectedSlot2Index + 1) % count;
    }

    // 決定キー（Z, Enter, Space）で決定
    if (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE)) {
        if (m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
            // スロット1 (ハンドガン) とスロット2 (選択武器) を登録
            PlayerSelectionManager::GetInstance().SetSlot1Weapon(m_slot1Weapon);
            PlayerSelectionManager::GetInstance().SetSlot2Weapon(m_slot2Options[m_selectedSlot2Index]);

            // ゲーム本編へ遷移
            SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::SOLO));
        }
    }
}

void WeaponSelectScene::Draw() {
    DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);

    FontManager::GetInstance().DrawStringCenter(960, 60, "WEAPON SELECT", GetColor(255, 220, 0), 48);
    
    // 選択済キャラ情報
    const auto& selChar = PlayerSelectionManager::GetInstance().GetSelection().selectedCharacter;
    FontManager::GetInstance().DrawFormatString(120, 130, GetColor(0, 200, 255), 22, "SELECTED CHARACTER: [%s] (%s)", selChar.name.c_str(), selChar.typeName.c_str());

    // ----------------------------------------------------
    // SLOT 1 (固定装備: HANDGUN)
    // ----------------------------------------------------
    int slot1X = 120;
    int slot1Y = 180;
    int slot1W = 1680;
    int slot1H = 130;

    DrawBox(slot1X, slot1Y, slot1X + slot1W, slot1Y + slot1H, GetColor(22, 30, 45), TRUE);
    DrawBox(slot1X, slot1Y, slot1X + slot1W, slot1Y + slot1H, GetColor(0, 160, 230), FALSE);

    // 固定表示タグ
    DrawBox(slot1X + 20, slot1Y + 20, slot1X + 160, slot1Y + 50, GetColor(0, 140, 220), TRUE);
    FontManager::GetInstance().DrawString(slot1X + 35, slot1Y + 25, "SLOT 1 [固定]", GetColor(255, 255, 255), 16);

    FontManager::GetInstance().DrawFormatString(slot1X + 180, slot1Y + 22, GetColor(255, 255, 255), 24, "【%s】 (%s)", m_slot1Weapon.name.c_str(), m_slot1Weapon.category.c_str());
    FontManager::GetInstance().DrawString(slot1X + 180, slot1Y + 65, m_slot1Weapon.description.c_str(), GetColor(180, 190, 210), 18);
    FontManager::GetInstance().DrawFormatString(slot1X + 1200, slot1Y + 40, GetColor(255, 220, 40), 20, "威力: %d  /  装弾数: %d", m_slot1Weapon.damage, m_slot1Weapon.maxAmmo);

    // ----------------------------------------------------
    // SLOT 2 (自由選択武器)
    // ----------------------------------------------------
    FontManager::GetInstance().DrawString(120, 345, "SLOT 2 : サブ武器を選択してください ( [←][→] / [A][D] で選択、[Z] / [Enter] で出撃 )", GetColor(255, 220, 0), 22);

    int count = static_cast<int>(m_slot2Options.size());
    int cardW = 520;
    int cardH = 340;
    int gap = 40;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 400;

    for (int i = 0; i < count; i++) {
        const auto& wpn = m_slot2Options[i];
        int cardX = startX + i * (cardW + gap);
        bool isSelected = (i == m_selectedSlot2Index);

        if (isSelected) {
            DrawBox(cardX - 4, startY - 4, cardX + cardW + 4, startY + cardH + 4, GetColor(255, 200, 0), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(35, 45, 65), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(255, 255, 255), FALSE);
        } else {
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(20, 26, 38), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(60, 75, 95), FALSE);
        }

        // 武器タイトルヘッダー
        DrawBox(cardX + 20, startY + 20, cardX + cardW - 20, startY + 65, isSelected ? GetColor(0, 140, 220) : GetColor(35, 45, 60), TRUE);
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + 30, wpn.name.c_str(), GetColor(255, 255, 255), 24);

        // 武器詳細
        int textY = startY + 85;
        FontManager::GetInstance().DrawFormatString(cardX + 40, textY, GetColor(150, 190, 220), 18, "カテゴリ  : %s", wpn.category.c_str());
        textY += 32;
        FontManager::GetInstance().DrawFormatString(cardX + 40, textY, GetColor(255, 100, 100), 18, "単発威力  : %d", wpn.damage);
        textY += 32;
        FontManager::GetInstance().DrawFormatString(cardX + 40, textY, GetColor(255, 220, 40), 18, "装弾数    : %d 発", wpn.maxAmmo);
        textY += 45;

        // 特徴説明
        DrawBox(cardX + 30, textY, cardX + cardW - 30, textY + 60, GetColor(12, 16, 24), TRUE);
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY + 18, wpn.description.c_str(), GetColor(200, 210, 225), 15);

        if (isSelected) {
            DrawBox(cardX + 30, startY + cardH - 50, cardX + cardW - 30, startY + cardH - 15, GetColor(255, 200, 0), TRUE);
            FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + cardH - 41, "【 出撃決定 】 [ Z / ENTER ]", GetColor(0, 0, 0), 18);
        }
    }

    // 出撃確認サマリー情報
    if (m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
        const auto& selSlot2 = m_slot2Options[m_selectedSlot2Index];
        int summaryY = 780;
        DrawBox(200, summaryY, 1720, summaryY + 110, GetColor(25, 35, 55), TRUE);
        DrawBox(200, summaryY, 1720, summaryY + 110, GetColor(0, 200, 255), FALSE);

        FontManager::GetInstance().DrawString(240, summaryY + 18, "【最終出撃装備】", GetColor(255, 220, 0), 20);
        FontManager::GetInstance().DrawFormatString(240, summaryY + 55, GetColor(255, 255, 255), 22,
            "キャラ: %s   |   Slot 1: %s   |   Slot 2: %s",
            selChar.name.c_str(), m_slot1Weapon.name.c_str(), selSlot2.name.c_str()
        );
    }
}

void WeaponSelectScene::Finalize() {
}
