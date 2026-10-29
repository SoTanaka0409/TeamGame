#include "CharacterSelectScene.h"
#include "WeaponSelectScene.h"
#include "TitleScene.h"
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
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    m_bgImageIndex = Load2DImage("character_select_bg.png");
    if (m_bgImageIndex == -1) {
        m_bgImageIndex = Load2DImage("select_bg.png");
    }
    m_windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // UITextWindow による詳細説明枠の設定
    m_detailWindow.SetFixedSize(1200, 110);
    m_detailWindow.SetMaxTextWidth(1140);
    m_detailWindow.SetPadding(UI::Margin(30, 16, 30, 16));
    m_detailWindow.SetLineSpacing(6);
    if (m_windowFrameIndex != -1) {
        m_detailWindow.SetBgGraphHandle(Get2DImage(m_windowFrameIndex));
    }

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

    // ESCキーでタイトル画面へ戻る
    if (input.IsKeyPressed(KEY_INPUT_ESCAPE)) {
        auto titleScene = std::make_shared<TitleScene>();
        titleScene->state = TitleScene::TitleState::MODE_SELECT;
        titleScene->cursor = 0;
        SceneManager::GetInstance().ChangeScene(titleScene);
    }
}

void CharacterSelectScene::Draw() {
    // 背景画像の描画（配列番号から呼び出し）
    if (m_bgImageIndex != -1) {
        DrawFullScreen2DImage(m_bgImageIndex, false);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    } else {
        DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);
    }

    FontManager::GetInstance().DrawStringCenter(960, 60, "CHARACTER SELECT", GetColor(255, 220, 0), 48);
    FontManager::GetInstance().DrawStringCenter(960, 125, "使用するキャラクターを選択してください ( [←][→] / [A][D] で移動、[Z] / [Enter] で決定、[Esc] で戻る )", GetColor(200, 210, 225), 20);

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
        textY += 24;
        FontManager::GetInstance().DrawFormatString(cardX + 25, textY, GetColor(255, 215, 0), 16, "SKILL   : %s", chara.skillName.c_str());
        textY += 28;

        // 選択案内
        if (isSelected) {
            DrawBox(cardX + 15, startY + cardH - 45, cardX + cardW - 15, startY + cardH - 12, chara.themeColor, TRUE);
            FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + cardH - 37, "[ Z / ENTER ] 選択", GetColor(0, 0, 0), 16);
        }
    }

    // 画面下部の詳細説明枠 (キャラクターの性能・特徴・スキルを確実に表示)
    if (m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];
        int infoX = 360;
        int infoY = 665;
        int infoW = 1200;
        int infoH = 120;

        // 背景半透明パネル
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(infoX, infoY, infoX + infoW, infoY + infoH, GetColor(20, 28, 42), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(infoX, infoY, infoX + infoW, infoY + infoH, curChara.themeColor, FALSE);

        // ヘッダー（名前とタイプ）
        FontManager::GetInstance().DrawFormatString(infoX + 35, infoY + 16, GetColor(255, 220, 0), 22, "【%s】  タイプ: %s", curChara.name.c_str(), curChara.typeName.c_str());
        DrawLine(infoX + 35, infoY + 48, infoX + infoW - 35, infoY + 48, GetColor(60, 90, 130));

        // 本文詳細説明文
        FontManager::GetInstance().DrawString(infoX + 35, infoY + 62, curChara.description.c_str(), GetColor(225, 235, 245), 20);
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
