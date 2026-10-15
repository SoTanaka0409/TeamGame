#include "CharacterSelectScene.h"
#include "WeaponSelectScene.h"
#include "../Managers/SceneManager.h"
#include "../Managers/FontManager.h"
#include "../Managers/InputManager.h"
#include <DxLib.h>

CharacterSelectScene::CharacterSelectScene() {
}

CharacterSelectScene::~CharacterSelectScene() {
    Finalize();
}

void CharacterSelectScene::Init() {
    m_characters = PlayerSelectionManager::GetAvailableCharacters();
    m_selectedIndex = 0;

    // プレビュー画像のロード
    m_previewHandles.clear();
    for (const auto& chara : m_characters) {
        int handle = -1;
        if (!chara.previewPath.empty()) {
            handle = LoadGraph(chara.previewPath.c_str());
        }
        m_previewHandles.push_back(handle);
    }
}

void CharacterSelectScene::Update() {
    int count = static_cast<int>(m_characters.size());
    auto& input = InputManager::GetInstance();

    if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A)) {
        m_selectedIndex = (m_selectedIndex - 1 + count) % count;
    }
    if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D)) {
        m_selectedIndex = (m_selectedIndex + 1) % count;
    }

    if (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE)) {
        if (m_selectedIndex >= 0 && m_selectedIndex < count) {
            PlayerSelectionManager::GetInstance().SetSelectedCharacter(m_characters[m_selectedIndex]);
            SceneManager::GetInstance().ChangeScene(std::make_shared<WeaponSelectScene>());
        }
    }
}

void CharacterSelectScene::Draw() {
    DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);
    FontManager::GetInstance().DrawStringCenter(960, 60, "CHARACTER SELECT", GetColor(255, 220, 0), 48);
    FontManager::GetInstance().DrawStringCenter(960, 125, "使用するキャラクターを選択してください ( [←][→] / [A][D] で移動、[Z] / [Enter] で決定 )", GetColor(200, 210, 225), 20);

    int count = static_cast<int>(m_characters.size());
    int cardW = 270;
    int cardH = 430;
    int gap = 30;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 180;

    for (int i = 0; i < count; i++) {
        const auto& chara = m_characters[i];
        int cardX = startX + i * (cardW + gap);
        bool isSelected = (i == m_selectedIndex);

        if (isSelected) {
            DrawBox(cardX - 4, startY - 4, cardX + cardW + 4, startY + cardH + 4, chara.themeColor, TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(30, 40, 60), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(255, 255, 255), FALSE);
        } else {
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(20, 25, 35), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(60, 75, 95), FALSE);
        }

        // アバター領域
        int avatarMargin = 16;
        int avatarH = 160;
        int avX1 = cardX + avatarMargin;
        int avY1 = startY + avatarMargin;
        int avX2 = cardX + cardW - avatarMargin;
        int avY2 = avY1 + avatarH;

        DrawBox(avX1, avY1, avX2, avY2, GetColor(10, 15, 22), TRUE);
        DrawBox(avX1, avY1, avX2, avY2, chara.themeColor, FALSE);

        if (i < static_cast<int>(m_previewHandles.size()) && m_previewHandles[i] != -1) {
            // 登録プレビュー画像の描画
            DrawExtendGraph(avX1 + 1, avY1 + 1, avX2 - 1, avY2 - 1, m_previewHandles[i], TRUE);
        } else {
            // 画像がない場合はIDアイコン
            int circleX = cardX + cardW / 2;
            int circleY = avY1 + avatarH / 2;
            DrawCircle(circleX, circleY, 36, chara.themeColor, TRUE);
            FontManager::GetInstance().DrawFormatString(circleX - 8, circleY - 12, GetColor(255, 255, 255), 24, "%d", chara.id);
        }

        // キャラ情報テキスト
        int textY = avY2 + 15;
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, chara.name.c_str(), isSelected ? GetColor(255, 240, 100) : GetColor(230, 230, 230), 22);
        textY += 30;

        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, chara.typeName.c_str(), GetColor(150, 180, 210), 16);
        textY += 32;

        // ステータス表示
        FontManager::GetInstance().DrawFormatString(cardX + 25, textY, GetColor(100, 255, 100), 16, "HP      : %d", chara.maxHp);
        textY += 24;
        FontManager::GetInstance().DrawFormatString(cardX + 25, textY, GetColor(100, 200, 255), 16, "SPEED   : %.1f", chara.moveSpeed);
        textY += 35;

        // 選択案内
        if (isSelected) {
            DrawBox(cardX + 15, startY + cardH - 45, cardX + cardW - 15, startY + cardH - 12, chara.themeColor, TRUE);
            FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + cardH - 37, "[ Z / ENTER ] 選択", GetColor(0, 0, 0), 16);
        }
    }

    // 画面下部の詳細説明枠
    if (m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];
        int infoY = 670;
        DrawBox(360, infoY, 1560, infoY + 110, GetColor(25, 32, 45), TRUE);
        DrawBox(360, infoY, 1560, infoY + 110, curChara.themeColor, FALSE);

        FontManager::GetInstance().DrawFormatString(390, infoY + 18, GetColor(255, 220, 0), 20, "【%s】 %s", curChara.name.c_str(), curChara.typeName.c_str());
        FontManager::GetInstance().DrawString(390, infoY + 55, curChara.description.c_str(), GetColor(220, 230, 240), 18);
    }
}

void CharacterSelectScene::Finalize() {
    for (int h : m_previewHandles) {
        if (h != -1) {
            DeleteGraph(h);
        }
    }
    m_previewHandles.clear();
}
