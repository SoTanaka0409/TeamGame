#include "CharacterSelectScene.h"
#include "WeaponSelectScene.h"
#include "../Managers/SceneManager.h"
#include "../Managers/FontManager.h"
#include "../Managers/InputManager.h"
#include <DxLib.h>

CharacterSelectScene::CharacterSelectScene() {
}

void CharacterSelectScene::Init() {
    m_characters = PlayerSelectionManager::GetAvailableCharacters();
    m_selectedIndex = 0;
}

void CharacterSelectScene::Update() {
    int count = static_cast<int>(m_characters.size());
    auto& input = InputManager::GetInstance();

    // 左右・WASD操作でカーソル移動
    if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A)) {
        m_selectedIndex = (m_selectedIndex - 1 + count) % count;
    }
    if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D)) {
        m_selectedIndex = (m_selectedIndex + 1) % count;
    }

    // 決定キー（Z, Enter, Space）で確定
    if (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE)) {
        if (m_selectedIndex >= 0 && m_selectedIndex < count) {
            // 選択したキャラデータを登録
            PlayerSelectionManager::GetInstance().SetSelectedCharacter(m_characters[m_selectedIndex]);
            // 武器選択画面へ遷移
            SceneManager::GetInstance().ChangeScene(std::make_shared<WeaponSelectScene>());
        }
    }
}

void CharacterSelectScene::Draw() {
    // 暗めの背景
    DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);

    // タイトル描画 (FontManager使用)
    FontManager::GetInstance().DrawStringCenter(960, 70, "CHARACTER SELECT", GetColor(255, 220, 0), 48);
    FontManager::GetInstance().DrawStringCenter(960, 135, "使用するキャラクターを選択してください ( [←][→] / [A][D] で移動、[Z] / [Enter] で確定 )", GetColor(200, 210, 225), 20);

    int count = static_cast<int>(m_characters.size());
    int cardW = 270;
    int cardH = 430;
    int gap = 30;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 220;

    for (int i = 0; i < count; i++) {
        const auto& chara = m_characters[i];
        int cardX = startX + i * (cardW + gap);
        bool isSelected = (i == m_selectedIndex);

        // カード外枠＆背景
        if (isSelected) {
            // 選択中カードのハイライト背景
            DrawBox(cardX - 4, startY - 4, cardX + cardW + 4, startY + cardH + 4, chara.themeColor, TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(30, 40, 60), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(255, 255, 255), FALSE);
        } else {
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(20, 25, 35), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(60, 75, 95), FALSE);
        }

        // キャラのアバター領域
        int avatarMargin = 18;
        int avatarH = 160;
        DrawBox(cardX + avatarMargin, startY + avatarMargin, cardX + cardW - avatarMargin, startY + avatarMargin + avatarH, GetColor(10, 15, 22), TRUE);
        DrawBox(cardX + avatarMargin, startY + avatarMargin, cardX + cardW - avatarMargin, startY + avatarMargin + avatarH, chara.themeColor, FALSE);
        
        // アバター中央にID描画
        int circleX = cardX + cardW / 2;
        int circleY = startY + avatarMargin + avatarH / 2;
        DrawCircle(circleX, circleY, 40, chara.themeColor, TRUE);
        FontManager::GetInstance().DrawFormatString(circleX - 8, circleY - 12, GetColor(255, 255, 255), 24, "%d", chara.id);

        // キャラ情報表示
        int textY = startY + avatarMargin + avatarH + 20;
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, chara.name.c_str(), isSelected ? GetColor(255, 240, 100) : GetColor(230, 230, 230), 22);
        textY += 32;

        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, chara.typeName.c_str(), GetColor(150, 180, 210), 16);
        textY += 35;

        // ステータス表示
        FontManager::GetInstance().DrawFormatString(cardX + 25, textY, GetColor(100, 255, 100), 16, "HP      : %d", chara.maxHp);
        textY += 24;
        FontManager::GetInstance().DrawFormatString(cardX + 25, textY, GetColor(100, 200, 255), 16, "SPEED   : %.1f", chara.moveSpeed);
        textY += 36;

        // 選択案内
        if (isSelected) {
            DrawBox(cardX + 15, startY + cardH - 50, cardX + cardW - 15, startY + cardH - 15, chara.themeColor, TRUE);
            FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + cardH - 41, "[ Z / ENTER ] 決定", GetColor(0, 0, 0), 16);
        }
    }

    // 画面下部の詳細説明
    if (m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];
        int infoY = 710;
        DrawBox(360, infoY, 1560, infoY + 120, GetColor(25, 32, 45), TRUE);
        DrawBox(360, infoY, 1560, infoY + 120, curChara.themeColor, FALSE);

        FontManager::GetInstance().DrawFormatString(390, infoY + 20, GetColor(255, 220, 0), 20, "【%s】 %s", curChara.name.c_str(), curChara.typeName.c_str());
        FontManager::GetInstance().DrawString(390, infoY + 60, curChara.description.c_str(), GetColor(220, 230, 240), 18);
    }
}

void CharacterSelectScene::Finalize() {
}
