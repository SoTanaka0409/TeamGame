#include "CharacterSelectScene.h"
#include "SoundManager.h"
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
    // タイトル〜キャラ選択までBGMを継続再生
    SoundManager::GetInstance().PlayBGM("title_bgm", 0.85f);
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
    m_state = SelectState::SELECTING;
    m_confirmCursor = 0;
    m_hasReleasedConfirm = false;

    // プレビュー画像のロード
    m_previewHandles.clear();
    for (const auto& chara : m_characters) {
        int handle = -1;
        if (!chara.previewPath.empty()) {
            handle = LoadGraph(chara.previewPath.c_str());
            if (handle == -1) {
                // 実行ディレクトリ差分およびASCII別名のフォールバック
                std::vector<std::string> fallbacks = {
                    "TeamGame/" + chara.previewPath,
                    "../" + chara.previewPath,
                    "../../" + chara.previewPath,
                    "Resource/Images/Characters/character_" + std::to_string(chara.id) + ".png",
                    "TeamGame/Resource/Images/Characters/character_" + std::to_string(chara.id) + ".png",
                    "Resource/GreenCharacter/preview.png"
                };
                for (const auto& fb : fallbacks) {
                    handle = LoadGraph(fb.c_str());
                    if (handle != -1) break;
                }
            }
        }
        m_previewHandles.push_back(handle);
    }
}

void CharacterSelectScene::Update() {
    int count = static_cast<int>(m_characters.size());
    auto& input = InputManager::GetInstance();

    // マウス座標の取得（ウィンドウ拡縮対応）
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080)) {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }

    bool currMouseLeft = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool mouseClicked = (currMouseLeft && !m_prevMouseLeft);
    bool mouseMoved = (m_prevMouseX != -1 && (mouseX != m_prevMouseX || mouseY != m_prevMouseY));

    // 誤進行防止: 決定キーが一度離されていることを検知
    bool anyConfirmHeld = (input.IsKeyHeld(KEY_INPUT_Z) || input.IsKeyHeld(KEY_INPUT_RETURN) || input.IsKeyHeld(KEY_INPUT_SPACE) || currMouseLeft);
    if (!anyConfirmHeld) {
        m_hasReleasedConfirm = true;
    }

    if (m_state == SelectState::SELECTING) {
        // --- 画面左上: タイトルに戻るボタン ---
        const int backBtnX = 50;
        const int backBtnY = 40;
        const int backBtnW = 220;
        const int backBtnH = 48;
        if (mouseX >= backBtnX && mouseX <= backBtnX + backBtnW &&
            mouseY >= backBtnY && mouseY <= backBtnY + backBtnH) {
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                auto titleScene = std::make_shared<TitleScene>();
                titleScene->state = TitleScene::TitleState::MAIN;
                titleScene->cursor = 0;
                SceneManager::GetInstance().ChangeScene(titleScene);
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // --- キャラクターカードのタップ（マウスクリック＆ホバー）判定 ---
        int cardW = 282;
        int cardH = 430;
        int gap = 22;
        int totalW = count * cardW + (count - 1) * gap;
        int startX = (1920 - totalW) / 2;
        int startY = 175;

        for (int i = 0; i < count; i++) {
            int cardX = startX + i * (cardW + gap);
            if (mouseX >= cardX && mouseX <= cardX + cardW &&
                mouseY >= startY && mouseY <= startY + cardH) {
                if (mouseMoved && m_selectedIndex != i) {
                    m_selectedIndex = i;
                }
                if (mouseClicked && m_hasReleasedConfirm) {
                    m_selectedIndex = i;
                    SoundManager::GetInstance().PlayTapSE();
                    m_state = SelectState::CONFIRMING;
                    m_confirmCursor = 0;
                    m_hasReleasedConfirm = false;
                }
            }
        }

        // --- キーボード操作 (← / → / A / D で選択) ---
        if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A)) {
            m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        }
        if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D)) {
            m_selectedIndex = (m_selectedIndex + 1) % count;
        }

        // 決定キー（Z, Enter, Space）で確認画面へ
        if (m_hasReleasedConfirm && (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE))) {
            if (m_selectedIndex >= 0 && m_selectedIndex < count) {
                SoundManager::GetInstance().PlayTapSE();
                m_state = SelectState::CONFIRMING;
                m_confirmCursor = 0;
                m_hasReleasedConfirm = false;
            }
        }

        // ESCキーでタイトル画面へ戻る
        if (input.IsKeyPressed(KEY_INPUT_ESCAPE)) {
            auto titleScene = std::make_shared<TitleScene>();
            titleScene->state = TitleScene::TitleState::MAIN;
            titleScene->cursor = 0;
            SceneManager::GetInstance().ChangeScene(titleScene);
            m_prevMouseLeft = currMouseLeft;
            m_prevMouseX = mouseX;
            m_prevMouseY = mouseY;
            return;
        }
    }
    else if (m_state == SelectState::CONFIRMING) {
        // --- 確認ダイアログ内のボタン当たり判定 ---
        const int modalW = 860;
        const int modalH = 500;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        const int btnW = 340;
        const int btnH = 58;
        const int btnY = modalY + modalH - 85;
        const int btn0X = modalX + 60;
        const int btn1X = modalX + modalW - 60 - btnW;

        // ボタン0: 決定して武器選択へ (YES)
        if (mouseX >= btn0X && mouseX <= btn0X + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (mouseMoved && m_confirmCursor != 0) m_confirmCursor = 0;
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                PlayerSelectionManager::GetInstance().SetSelectedCharacter(m_characters[m_selectedIndex]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<WeaponSelectScene>());
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // ボタン1: 選び直す (CANCEL)
        if (mouseX >= btn1X && mouseX <= btn1X + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (mouseMoved && m_confirmCursor != 1) m_confirmCursor = 1;
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                m_state = SelectState::SELECTING;
                m_hasReleasedConfirm = false;
            }
        }

        // --- キーボード操作 (← / → / A / D で選択肢切り替え) ---
        if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_RIGHT) ||
            input.IsKeyPressed(KEY_INPUT_A) || input.IsKeyPressed(KEY_INPUT_D)) {
            m_confirmCursor = (m_confirmCursor == 0) ? 1 : 0;
        }

        // Enter / Z / Space で決定
        if (m_hasReleasedConfirm && (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE))) {
            SoundManager::GetInstance().PlayTapSE();
            if (m_confirmCursor == 0) {
                PlayerSelectionManager::GetInstance().SetSelectedCharacter(m_characters[m_selectedIndex]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<WeaponSelectScene>());
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            } else {
                m_state = SelectState::SELECTING;
                m_hasReleasedConfirm = false;
            }
        }

        // ESCキーで確認をキャンセルして選択に戻る
        if (input.IsKeyPressed(KEY_INPUT_ESCAPE)) {
            SoundManager::GetInstance().PlayTapSE();
            m_state = SelectState::SELECTING;
            m_hasReleasedConfirm = false;
        }
    }

    m_prevMouseX = mouseX;
    m_prevMouseY = mouseY;
    m_prevMouseLeft = currMouseLeft;
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

    // 画面左上: タイトルに戻るボタンの描画
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);
    int drawW = 1920, drawH = 1080;
    GetDrawScreenSize(&drawW, &drawH);
    if (drawW > 0 && drawH > 0 && (drawW != 1920 || drawH != 1080)) {
        mouseX = static_cast<int>(mouseX * (1920.0f / drawW));
        mouseY = static_cast<int>(mouseY * (1080.0f / drawH));
    }
    const int backBtnX = 50;
    const int backBtnY = 40;
    const int backBtnW = 220;
    const int backBtnH = 48;
    bool isBackHovered = (mouseX >= backBtnX && mouseX <= backBtnX + backBtnW && mouseY >= backBtnY && mouseY <= backBtnY + backBtnH);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, isBackHovered ? 240 : 180);
    DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, isBackHovered ? GetColor(35, 50, 75) : GetColor(18, 25, 38), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, isBackHovered ? GetColor(0, 220, 255) : GetColor(60, 85, 115), FALSE);
    FontManager::GetInstance().DrawStringCenter(backBtnX + backBtnW / 2, backBtnY + 12, "← タイトルへ戻る", isBackHovered ? GetColor(255, 235, 80) : GetColor(200, 215, 230), 18);

    FontManager::GetInstance().DrawStringCenter(960, 60, "CHARACTER SELECT", GetColor(255, 220, 0), 48);
    FontManager::GetInstance().DrawStringCenter(960, 125, "使用するキャラクターを選択してください ( [←][→] または クリック/タップ で選択、[Enter] で決定 )", GetColor(200, 210, 225), 20);

    int count = static_cast<int>(m_characters.size());
    int cardW = 282;
    int cardH = 430;
    int gap = 22;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 175;

    for (int i = 0; i < count; i++) {
        const auto& chara = m_characters[i];
        int cardX = startX + i * (cardW + gap);
        bool isSelected = (i == m_selectedIndex);

        if (isSelected) {
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(32, 44, 68), TRUE);
            // 均一なセレクション太枠線
            DrawBox(cardX - 3, startY - 3, cardX + cardW + 3, startY + cardH + 3, chara.themeColor, FALSE);
            DrawBox(cardX - 2, startY - 2, cardX + cardW + 2, startY + cardH + 2, chara.themeColor, FALSE);
            DrawBox(cardX - 1, startY - 1, cardX + cardW + 1, startY + cardH + 1, GetColor(255, 255, 255), FALSE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, chara.themeColor, FALSE);
        } else {
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(20, 26, 38), TRUE);
            DrawBox(cardX, startY, cardX + cardW, startY + cardH, GetColor(60, 75, 95), FALSE);
        }

        // アバター領域
        int avatarMargin = 16;
        int avatarH = 160;
        int avX1 = cardX + avatarMargin;
        int avY1 = startY + avatarMargin;
        int avX2 = cardX + cardW - avatarMargin;
        int avY2 = avY1 + avatarH;

        DrawBox(avX1, avY1, avX2, avY2, GetColor(12, 16, 24), TRUE);

        if (i < static_cast<int>(m_previewHandles.size()) && m_previewHandles[i] != -1) {
            int imgW = 0, imgH = 0;
            GetGraphSize(m_previewHandles[i], &imgW, &imgH);
            if (imgW > 0 && imgH > 0) {
                float targetW = static_cast<float>(avX2 - avX1 - 4);
                float targetH = static_cast<float>(avY2 - avY1 - 4);
                float scale = (std::min)(targetW / imgW, targetH / imgH);
                int fitW = static_cast<int>(imgW * scale);
                int fitH = static_cast<int>(imgH * scale);
                int fitX = avX1 + 2 + static_cast<int>((targetW - fitW) / 2.0f);
                int fitY = avY1 + 2 + static_cast<int>((targetH - fitH) / 2.0f);
                DrawExtendGraph(fitX, fitY, fitX + fitW, fitY + fitH, m_previewHandles[i], TRUE);
            } else {
                DrawExtendGraph(avX1 + 1, avY1 + 1, avX2 - 1, avY2 - 1, m_previewHandles[i], TRUE);
            }
        } else {
            int circleX = cardX + cardW / 2;
            int circleY = avY1 + avatarH / 2;
            DrawCircle(circleX, circleY, 36, chara.themeColor, TRUE);
            FontManager::GetInstance().DrawFormatString(circleX - 8, circleY - 12, GetColor(255, 255, 255), 24, "%d", chara.id);
        }
        DrawBox(avX1, avY1, avX2, avY2, chara.themeColor, FALSE);

        // キャラ情報テキスト
        int textY = avY2 + 15;
        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, chara.name.c_str(), isSelected ? GetColor(255, 240, 100) : GetColor(230, 230, 230), 22);
        textY += 30;

        FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, textY, chara.typeName.c_str(), GetColor(150, 180, 210), 16);
        textY += 32;

        FontManager::GetInstance().DrawFormatString(cardX + 18, textY, GetColor(100, 255, 100), 16, "HP      : %d", chara.maxHp);
        textY += 24;
        FontManager::GetInstance().DrawFormatString(cardX + 18, textY, GetColor(100, 200, 255), 16, "SPEED   : %.1f", chara.moveSpeed);
        textY += 24;

        int skillFontSize = (chara.skillName.length() > 16) ? 14 : 16;
        FontManager::GetInstance().DrawFormatString(cardX + 18, textY, GetColor(255, 215, 0), skillFontSize, "SKILL   : %s", chara.skillName.c_str());
        textY += 28;

        // 選択案内
        if (isSelected) {
            DrawBox(cardX + 15, startY + cardH - 45, cardX + cardW - 15, startY + cardH - 12, chara.themeColor, TRUE);
            FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + cardH - 37, "[ タップ / ENTER ] 決定", GetColor(0, 0, 0), 16);
        }
    }

    // 画面下部の詳細説明枠
    if (m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];
        int infoX = startX;
        int infoY = startY + cardH + 28;
        int infoW = totalW;
        int infoH = 135;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
        DrawBox(infoX, infoY, infoX + infoW, infoY + infoH, GetColor(20, 28, 44), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(infoX, infoY, infoX + infoW, infoY + infoH, curChara.themeColor, FALSE);
        DrawBox(infoX + 1, infoY + 1, infoX + infoW - 1, infoY + infoH - 1, curChara.themeColor, FALSE);

        FontManager::GetInstance().DrawFormatString(infoX + 35, infoY + 16, GetColor(255, 220, 0), 22, "【%s】  タイプ: %s", curChara.name.c_str(), curChara.typeName.c_str());
        DrawLine(infoX + 35, infoY + 48, infoX + infoW - 35, infoY + 48, GetColor(60, 90, 130));
        FontManager::GetInstance().DrawString(infoX + 35, infoY + 62, curChara.description.c_str(), GetColor(225, 235, 245), 20);
    }

    // ========================================================
    // 決定確認モーダルダイアログ (CONFIRMING 状態)
    // ========================================================
    if (m_state == SelectState::CONFIRMING && m_selectedIndex >= 0 && m_selectedIndex < count) {
        const auto& curChara = m_characters[m_selectedIndex];

        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        DrawBox(0, 0, 1920, 1080, GetColor(6, 10, 16), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインモーダル枠
        const int modalW = 860;
        const int modalH = 500;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
        DrawBox(modalX, modalY, modalX + modalW, modalY + modalH, GetColor(16, 22, 34), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(modalX, modalY, modalX + modalW, modalY + modalH, GetColor(0, 180, 255), FALSE);
        DrawBox(modalX + 2, modalY + 2, modalX + modalW - 2, modalY + modalH - 2, GetColor(0, 120, 180), FALSE);
        DrawBox(modalX + 4, modalY + 4, modalX + modalW - 4, modalY + modalH - 4, GetColor(30, 50, 80), FALSE);

        // ヘッダータイトル
        int headerY = modalY + 24;
        FontManager::GetInstance().DrawStringCenter(960, headerY, "CONFIRM CHARACTER (キャラクター決定確認)", GetColor(255, 220, 0), 26);
        DrawLine(modalX + 45, headerY + 38, modalX + modalW - 45, headerY + 38, GetColor(0, 140, 200));

        // メッセージ
        FontManager::GetInstance().DrawStringCenter(960, headerY + 52, "このキャラクターで出撃します。よろしいですか？", GetColor(255, 255, 255), 22);

        // キャラクター情報カード
        int infoCardX = modalX + 45;
        int infoCardY = headerY + 90;
        int infoCardW = modalW - 90;
        int infoCardH = 205;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(infoCardX, infoCardY, infoCardX + infoCardW, infoCardY + infoCardH, GetColor(10, 15, 24), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(infoCardX, infoCardY, infoCardX + infoCardW, infoCardY + infoCardH, curChara.themeColor, FALSE);

        // アバター画像
        int avW = 160;
        int avH = infoCardH - 24;
        int avX = infoCardX + 16;
        int avY = infoCardY + 12;
        DrawBox(avX, avY, avX + avW, avY + avH, GetColor(18, 24, 34), TRUE);
        if (m_selectedIndex < static_cast<int>(m_previewHandles.size()) && m_previewHandles[m_selectedIndex] != -1) {
            int imgW = 0, imgH = 0;
            GetGraphSize(m_previewHandles[m_selectedIndex], &imgW, &imgH);
            if (imgW > 0 && imgH > 0) {
                float targetW = static_cast<float>(avW - 4);
                float targetH = static_cast<float>(avH - 4);
                float scale = (std::min)(targetW / imgW, targetH / imgH);
                int fitW = static_cast<int>(imgW * scale);
                int fitH = static_cast<int>(imgH * scale);
                int fitX = avX + 2 + static_cast<int>((targetW - fitW) / 2.0f);
                int fitY = avY + 2 + static_cast<int>((targetH - fitH) / 2.0f);
                DrawExtendGraph(fitX, fitY, fitX + fitW, fitY + fitH, m_previewHandles[m_selectedIndex], TRUE);
            } else {
                DrawExtendGraph(avX + 1, avY + 1, avX + avW - 1, avY + avH - 1, m_previewHandles[m_selectedIndex], TRUE);
            }
        } else {
            int circleX = avX + avW / 2;
            int circleY = avY + avH / 2;
            DrawCircle(circleX, circleY, 36, curChara.themeColor, TRUE);
            FontManager::GetInstance().DrawFormatString(circleX - 8, circleY - 12, GetColor(255, 255, 255), 24, "%d", curChara.id);
        }
        DrawBox(avX, avY, avX + avW, avY + avH, curChara.themeColor, FALSE);

        // キャラステータス & スキル
        int statX = avX + avW + 24;
        int statY = infoCardY + 16;
        FontManager::GetInstance().DrawFormatString(statX, statY, GetColor(255, 235, 80), 24, "%s  [%s]", curChara.name.c_str(), curChara.typeName.c_str());
        statY += 34;

        FontManager::GetInstance().DrawFormatString(statX, statY, GetColor(100, 255, 100), 18, "最大HP     : %d", curChara.maxHp);
        FontManager::GetInstance().DrawFormatString(statX + 220, statY, GetColor(100, 200, 255), 18, "移動速度   : %.1f", curChara.moveSpeed);
        statY += 30;

        FontManager::GetInstance().DrawFormatString(statX, statY, GetColor(255, 215, 0), 18, "スキル     : %s", curChara.skillName.c_str());
        statY += 28;

        FontManager::GetInstance().DrawString(statX, statY, curChara.description.c_str(), GetColor(210, 225, 240), 16);

        // 3. 2つのボタン (YES / CANCEL)
        const int btnW = 340;
        const int btnH = 58;
        const int btnY = modalY + modalH - 85;
        const int btn0X = modalX + 60;
        const int btn1X = modalX + modalW - 60 - btnW;

        // ボタン0: 決定して武器選択へ (YES)
        bool isSel0 = (m_confirmCursor == 0);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isSel0 ? 240 : 180);
        DrawBox(btn0X, btnY, btn0X + btnW, btnY + btnH, isSel0 ? GetColor(20, 50, 40) : GetColor(18, 28, 38), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(btn0X, btnY, btn0X + btnW, btnY + btnH, isSel0 ? GetColor(0, 255, 180) : GetColor(50, 90, 80), FALSE);
        if (isSel0) {
            DrawBox(btn0X + 1, btnY + 1, btn0X + btnW - 1, btnY + btnH - 1, GetColor(0, 255, 180), FALSE);
            DrawBox(btn0X, btnY, btn0X + 8, btnY + btnH, GetColor(0, 255, 180), TRUE);
        }
        FontManager::GetInstance().DrawStringCenter(btn0X + btnW / 2, btnY + 16, "✔  決定して武器選択へ (YES)", isSel0 ? GetColor(255, 255, 255) : GetColor(180, 220, 200), 20);

        // ボタン1: 選び直す (CANCEL)
        bool isSel1 = (m_confirmCursor == 1);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isSel1 ? 240 : 180);
        DrawBox(btn1X, btnY, btn1X + btnW, btnY + btnH, isSel1 ? GetColor(50, 25, 30) : GetColor(28, 20, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(btn1X, btnY, btn1X + btnW, btnY + btnH, isSel1 ? GetColor(255, 100, 100) : GetColor(80, 50, 60), FALSE);
        if (isSel1) {
            DrawBox(btn1X + 1, btnY + 1, btn1X + btnW - 1, btnY + btnH - 1, GetColor(255, 100, 100), FALSE);
            DrawBox(btn1X, btnY, btn1X + 8, btnY + btnH, GetColor(255, 100, 100), TRUE);
        }
        FontManager::GetInstance().DrawStringCenter(btn1X + btnW / 2, btnY + 16, "✕  選び直す (CANCEL)", isSel1 ? GetColor(255, 255, 255) : GetColor(220, 180, 180), 20);
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
