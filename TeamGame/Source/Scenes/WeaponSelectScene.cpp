#include "WeaponSelectScene.h"
#include "SoundManager.h"
#include "GameScene.h"
#include "CharacterSelectScene.h"
#include "../Managers/SceneManager.h"
#include "../Managers/FontManager.h"
#include "../Managers/InputManager.h"
#include <DxLib.h>

WeaponSelectScene::WeaponSelectScene() {
}

void WeaponSelectScene::Init() {
    // タイトル〜武器選択までBGMを継続再生
    SoundManager::GetInstance().PlayBGM("title_bgm", 0.85f);
    // =========================================================================
    // 【2D画像配列への読み込み & 番号管理】
    // 一度読み込むと内部配列にキャッシュされ、配列番号（インデックス）で何度でも呼び出せます。
    // =========================================================================
    m_bgImageIndex = Load2DImage("weapon_select_bg.png");
    if (m_bgImageIndex == -1) {
        m_bgImageIndex = Load2DImage("select_bg.png");
    }
    m_windowFrameIndex = Load2DImage("ui_window.png"); // 9スライス用枠画像

    // UITextWindow による出撃確認枠の設定 (読めるフォントサイズ22ptを設定)
    m_summaryWindow.SetFontHandle(FontManager::GetInstance().GetFont(22));
    m_summaryWindow.SetFixedSize(1520, 110);
    m_summaryWindow.SetMaxTextWidth(1440);
    m_summaryWindow.SetPadding(UI::Margin(40, 18, 40, 18));
    m_summaryWindow.SetLineSpacing(6);
    m_summaryWindow.SetBoxColor(GetColor(25, 35, 55), GetColor(0, 200, 255), 200);
    if (m_windowFrameIndex != -1) {
        m_summaryWindow.SetBgGraphHandle(Get2DImage(m_windowFrameIndex));
    }

    m_slot1Weapon = PlayerSelectionManager::GetFixedSlot1Weapon();
    m_slot2Options = PlayerSelectionManager::GetAvailableSlot2Weapons();
    m_selectedSlot2Index = 0;
    m_state = SelectState::SELECTING;
    m_confirmCursor = 0;
    m_hasReleasedConfirm = false;
}

void WeaponSelectScene::Update() {
    int count = static_cast<int>(m_slot2Options.size());
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

    // 誤進行防止
    bool anyConfirmHeld = (input.IsKeyHeld(KEY_INPUT_Z) || input.IsKeyHeld(KEY_INPUT_RETURN) || input.IsKeyHeld(KEY_INPUT_SPACE) || currMouseLeft);
    if (!anyConfirmHeld) {
        m_hasReleasedConfirm = true;
    }

    if (m_state == SelectState::SELECTING) {
        // --- 画面左上: キャラ選択に戻るボタン ---
        const int backBtnX = 50;
        const int backBtnY = 40;
        const int backBtnW = 250;
        const int backBtnH = 50;
        if (mouseX >= backBtnX && mouseX <= backBtnX + backBtnW &&
            mouseY >= backBtnY && mouseY <= backBtnY + backBtnH) {
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // --- 武器カードのタップ（マウスクリック＆ホバー）判定 ---
        int cardW = 540;
        int cardH = 350;
        int gap = 36;
        int totalW = count * cardW + (count - 1) * gap;
        int startX = (1920 - totalW) / 2;
        int startY = 400;

        for (int i = 0; i < count; i++) {
            int cardX = startX + i * (cardW + gap);
            if (mouseX >= cardX && mouseX <= cardX + cardW &&
                mouseY >= startY && mouseY <= startY + cardH) {
                if (mouseMoved && m_selectedSlot2Index != i) {
                    m_selectedSlot2Index = i;
                }
                if (mouseClicked && m_hasReleasedConfirm) {
                    m_selectedSlot2Index = i;
                    SoundManager::GetInstance().PlayTapSE();
                    m_state = SelectState::CONFIRMING;
                    m_confirmCursor = 0;
                    m_hasReleasedConfirm = false;
                }
            }
        }

        // --- キーボード操作 (左右 / A / D で選択) ---
        if (input.IsKeyPressed(KEY_INPUT_LEFT) || input.IsKeyPressed(KEY_INPUT_A)) {
            m_selectedSlot2Index = (m_selectedSlot2Index - 1 + count) % count;
        }
        if (input.IsKeyPressed(KEY_INPUT_RIGHT) || input.IsKeyPressed(KEY_INPUT_D)) {
            m_selectedSlot2Index = (m_selectedSlot2Index + 1) % count;
        }

        // 決定キー（Z, Enter, Space）で確認画面へ
        if (m_hasReleasedConfirm && (input.IsKeyPressed(KEY_INPUT_Z) || input.IsKeyPressed(KEY_INPUT_RETURN) || input.IsKeyPressed(KEY_INPUT_SPACE))) {
            if (m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
                SoundManager::GetInstance().PlayTapSE();
                m_state = SelectState::CONFIRMING;
                m_confirmCursor = 0;
                m_hasReleasedConfirm = false;
            }
        }

        // ESCキーでキャラクター選択画面へ戻る
        if (input.IsKeyPressed(KEY_INPUT_ESCAPE)) {
            SceneManager::GetInstance().ChangeScene(std::make_shared<CharacterSelectScene>());
            m_prevMouseLeft = currMouseLeft;
            m_prevMouseX = mouseX;
            m_prevMouseY = mouseY;
            return;
        }
    }
    else if (m_state == SelectState::CONFIRMING) {
        // --- 確認ダイアログ内のボタン当たり判定 ---
        const int modalW = 1040;
        const int modalH = 570;
        const int modalX = (1920 - modalW) / 2;
        const int modalY = (1080 - modalH) / 2;

        const int btnW = 400;
        const int btnH = 62;
        const int btnY = modalY + modalH - 92;
        const int btn0X = modalX + 70;
        const int btn1X = modalX + modalW - 70 - btnW;

        // ボタン0: 出撃開始 (START GAME)
        if (mouseX >= btn0X && mouseX <= btn0X + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (mouseMoved && m_confirmCursor != 0) m_confirmCursor = 0;
            if (mouseClicked && m_hasReleasedConfirm) {
                SoundManager::GetInstance().PlayTapSE();
                PlayerSelectionManager::GetInstance().SetSlot1Weapon(m_slot1Weapon);
                PlayerSelectionManager::GetInstance().SetSlot2Weapon(m_slot2Options[m_selectedSlot2Index]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::SOLO));
                m_prevMouseLeft = currMouseLeft;
                m_prevMouseX = mouseX;
                m_prevMouseY = mouseY;
                return;
            }
        }

        // ボタン1: 装備を変更 (CANCEL)
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
                PlayerSelectionManager::GetInstance().SetSlot1Weapon(m_slot1Weapon);
                PlayerSelectionManager::GetInstance().SetSlot2Weapon(m_slot2Options[m_selectedSlot2Index]);
                SceneManager::GetInstance().ChangeScene(std::make_shared<GameScene>(PlayMode::SOLO));
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

void WeaponSelectScene::Draw() {
    // 背景画像の描画（配列番号から呼び出し）
    if (m_bgImageIndex != -1) {
        DrawFullScreen2DImage(m_bgImageIndex, false);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    } else {
        DrawBox(0, 0, 1920, 1080, GetColor(15, 20, 30), TRUE);
    }

    // 画面左上: キャラ選択に戻るボタン
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
    const int backBtnW = 250;
    const int backBtnH = 50;
    bool isBackHovered = (mouseX >= backBtnX && mouseX <= backBtnX + backBtnW && mouseY >= backBtnY && mouseY <= backBtnY + backBtnH);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, isBackHovered ? 240 : 180);
    DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, isBackHovered ? GetColor(35, 50, 75) : GetColor(18, 25, 38), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, isBackHovered ? GetColor(0, 220, 255) : GetColor(60, 85, 115), FALSE);
    FontManager::GetInstance().DrawStringCenter(backBtnX + backBtnW / 2, backBtnY + 14, "← キャラ選択に戻る", isBackHovered ? GetColor(255, 235, 80) : GetColor(200, 215, 230), 20);

    FontManager::GetInstance().DrawStringCenter(960, 48, "WEAPON SELECT", GetColor(255, 220, 0), 48);
    FontManager::GetInstance().DrawStringCenter(960, 102, "( [←][→] または クリック/タップ でサブ武器を選択、[Enter] で出撃確認 )", GetColor(190, 205, 220), 21);
    
    // 選択済キャラ情報
    const auto& selChar = PlayerSelectionManager::GetInstance().GetSelection().selectedCharacter;
    FontManager::GetInstance().DrawFormatString(120, 136, GetColor(0, 200, 255), 22, "SELECTED CHARACTER: [%s] (%s)", selChar.name.c_str(), selChar.typeName.c_str());

    // ----------------------------------------------------
    // SLOT 1 (固定装備: HANDGUN)
    // ----------------------------------------------------
    int slot1X = 120;
    int slot1Y = 180;
    int slot1W = 1680;
    int slot1H = 134;

    DrawBox(slot1X, slot1Y, slot1X + slot1W, slot1Y + slot1H, GetColor(22, 30, 45), TRUE);
    DrawBox(slot1X, slot1Y, slot1X + slot1W, slot1Y + slot1H, GetColor(0, 160, 230), FALSE);

    // 固定表示タグ
    DrawBox(slot1X + 20, slot1Y + 20, slot1X + 175, slot1Y + 54, GetColor(0, 140, 220), TRUE);
    FontManager::GetInstance().DrawStringCenter(slot1X + 97, slot1Y + 27, "SLOT 1 [固定]", GetColor(255, 255, 255), 18);

    FontManager::GetInstance().DrawFormatString(slot1X + 195, slot1Y + 22, GetColor(255, 255, 255), 24, "【%s】 (%s)", m_slot1Weapon.name.c_str(), m_slot1Weapon.category.c_str());
    FontManager::GetInstance().DrawStringWrapped(slot1X + 195, slot1Y + 62, 980, m_slot1Weapon.description.c_str(), GetColor(190, 205, 225), 20, 4);
    FontManager::GetInstance().DrawFormatString(slot1X + 1230, slot1Y + 50, GetColor(255, 220, 40), 22, "威力: %d  /  装弾数: %d", m_slot1Weapon.damage, m_slot1Weapon.maxAmmo);

    // ----------------------------------------------------
    // SLOT 2 (自由選択武器)
    // ----------------------------------------------------
    FontManager::GetInstance().DrawString(120, 342, "SLOT 2 : サブ武器を選択してください ( [←][→] または クリック/タップ で選択、[Enter] で決定 )", GetColor(255, 220, 0), 22);

    int count = static_cast<int>(m_slot2Options.size());
    int cardW = 540;
    int cardH = 350;
    int gap = 36;
    int totalW = count * cardW + (count - 1) * gap;
    int startX = (1920 - totalW) / 2;
    int startY = 392;

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
        int textY = startY + 84;
        FontManager::GetInstance().DrawFormatString(cardX + 36, textY, GetColor(150, 190, 220), 20, "カテゴリ  : %s", wpn.category.c_str());
        textY += 32;
        FontManager::GetInstance().DrawFormatString(cardX + 36, textY, GetColor(255, 100, 100), 20, "単発威力  : %d", wpn.damage);
        textY += 32;
        FontManager::GetInstance().DrawFormatString(cardX + 36, textY, GetColor(255, 220, 40), 20, "装弾数    : %d 発", wpn.maxAmmo);
        textY += 40;

        // 特徴説明 (読める18ptで自動折り返し・はみ出し防止)
        DrawBox(cardX + 24, textY, cardX + cardW - 24, textY + 72, GetColor(12, 16, 24), TRUE);
        FontManager::GetInstance().DrawStringWrappedCenter(cardX + cardW / 2, textY + 12, cardW - 72, wpn.description.c_str(), GetColor(210, 220, 235), 18, 4);

        if (isSelected) {
            DrawBox(cardX + 30, startY + cardH - 52, cardX + cardW - 30, startY + cardH - 14, GetColor(255, 200, 0), TRUE);
            FontManager::GetInstance().DrawStringCenter(cardX + cardW / 2, startY + cardH - 43, "【 タップ / ENTER で出撃確認 】", GetColor(0, 0, 0), 19);
        }
    }

    // 出撃確認サマリー情報
    if (m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
        const auto& selSlot2 = m_slot2Options[m_selectedSlot2Index];
        int summaryX = 200;
        int summaryY = 776;

        char buf[256];
        sprintf_s(buf, sizeof(buf), "キャラ: %s   |   Slot 1: %s   |   Slot 2: %s",
            selChar.name.c_str(), m_slot1Weapon.name.c_str(), selSlot2.name.c_str());

        m_summaryWindow.SetTitle("【最終出撃装備】", GetColor(255, 220, 0));
        m_summaryWindow.SetText(buf);
        m_summaryWindow.Draw(summaryX, summaryY);
    }

    // ========================================================
    // 最終出撃確認モーダルダイアログ (CONFIRMING 状態)
    // ========================================================
    if (m_state == SelectState::CONFIRMING && m_selectedSlot2Index >= 0 && m_selectedSlot2Index < count) {
        const auto& selSlot2 = m_slot2Options[m_selectedSlot2Index];

        // 1. 全面暗転オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        DrawBox(0, 0, 1920, 1080, GetColor(6, 10, 16), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. メインモーダル枠 (1040 x 570)
        const int modalW = 1040;
        const int modalH = 570;
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
        FontManager::GetInstance().DrawStringCenter(960, headerY, "FINAL DEPLOYMENT CONFIRMATION (最終出撃確認)", GetColor(255, 220, 0), 28);
        DrawLine(modalX + 45, headerY + 40, modalX + modalW - 45, headerY + 40, GetColor(0, 140, 200));

        // メッセージ
        FontManager::GetInstance().DrawStringCenter(960, headerY + 54, "この装備で戦場に出撃します。よろしいですか？", GetColor(255, 255, 255), 22);

        // 出撃構成サマリーボックス
        int infoCardX = modalX + 45;
        int infoCardY = headerY + 94;
        int infoCardW = modalW - 90;
        int infoCardH = 264;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(infoCardX, infoCardY, infoCardX + infoCardW, infoCardY + infoCardH, GetColor(10, 15, 24), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(infoCardX, infoCardY, infoCardX + infoCardW, infoCardY + infoCardH, GetColor(0, 140, 200), FALSE);

        int labelW = 154;
        int textX = infoCardX + 20 + labelW + 22;
        int textMaxW = (infoCardX + infoCardW - 24) - textX;

        // 行1: 選択キャラクター
        int rowY = infoCardY + 18;
        DrawBox(infoCardX + 20, rowY, infoCardX + 20 + labelW, rowY + 38, GetColor(25, 45, 75), TRUE);
        DrawBox(infoCardX + 20, rowY, infoCardX + 20 + labelW, rowY + 38, GetColor(0, 180, 255), FALSE);
        FontManager::GetInstance().DrawStringCenter(infoCardX + 20 + labelW / 2, rowY + 8, "出撃キャラ", GetColor(0, 200, 255), 18);

        char charLine[256];
        snprintf(charLine, sizeof(charLine), "%s  [%s]  (最大HP: %d / 移動速度: %.1f)",
            selChar.name.c_str(), selChar.typeName.c_str(), selChar.maxHp, selChar.moveSpeed);
        int charFit = FontManager::GetInstance().GetFittingFontSize(charLine, textMaxW, 22, 18);
        FontManager::GetInstance().DrawString(textX, rowY + 6, charLine, GetColor(255, 235, 80), charFit);
        rowY += 50;

        DrawLine(infoCardX + 20, rowY, infoCardX + infoCardW - 20, rowY, GetColor(35, 50, 70));
        rowY += 14;

        // 行2: SLOT 1 武器
        DrawBox(infoCardX + 20, rowY, infoCardX + 20 + labelW, rowY + 38, GetColor(20, 35, 50), TRUE);
        DrawBox(infoCardX + 20, rowY, infoCardX + 20 + labelW, rowY + 38, GetColor(0, 140, 200), FALSE);
        FontManager::GetInstance().DrawStringCenter(infoCardX + 20 + labelW / 2, rowY + 8, "メイン [固定]", GetColor(180, 210, 240), 18);

        char slot1Line[256];
        snprintf(slot1Line, sizeof(slot1Line), "【%s】 (%s)  威力: %d  /  装弾数: %d",
            m_slot1Weapon.name.c_str(), m_slot1Weapon.category.c_str(), m_slot1Weapon.damage, m_slot1Weapon.maxAmmo);
        int slot1Fit = FontManager::GetInstance().GetFittingFontSize(slot1Line, textMaxW, 21, 18);
        FontManager::GetInstance().DrawString(textX, rowY + 6, slot1Line, GetColor(255, 255, 255), slot1Fit);
        rowY += 50;

        DrawLine(infoCardX + 20, rowY, infoCardX + infoCardW - 20, rowY, GetColor(35, 50, 70));
        rowY += 14;

        // 行3: SLOT 2 武器
        DrawBox(infoCardX + 20, rowY, infoCardX + 20 + labelW, rowY + 38, GetColor(45, 38, 20), TRUE);
        DrawBox(infoCardX + 20, rowY, infoCardX + 20 + labelW, rowY + 38, GetColor(255, 200, 0), FALSE);
        FontManager::GetInstance().DrawStringCenter(infoCardX + 20 + labelW / 2, rowY + 8, "サブ [選択]", GetColor(255, 220, 80), 18);

        char slot2Line[256];
        snprintf(slot2Line, sizeof(slot2Line), "【%s】 (%s)  威力: %d  /  装弾数: %d",
            selSlot2.name.c_str(), selSlot2.category.c_str(), selSlot2.damage, selSlot2.maxAmmo);
        int slot2Fit = FontManager::GetInstance().GetFittingFontSize(slot2Line, textMaxW, 21, 18);
        FontManager::GetInstance().DrawString(textX, rowY + 4, slot2Line, GetColor(255, 240, 100), slot2Fit);

        std::string slot2Desc = std::string("※ ") + selSlot2.description;
        FontManager::GetInstance().DrawStringWrapped(textX, rowY + 34, textMaxW, slot2Desc.c_str(), GetColor(190, 210, 230), 19, 4);

        // 3. 2つのボタン (YES / CANCEL)
        const int btnW = 400;
        const int btnH = 62;
        const int btnY = modalY + modalH - 92;
        const int btn0X = modalX + 70;
        const int btn1X = modalX + modalW - 70 - btnW;

        // ボタン0: 出撃開始 (START GAME)
        bool isSel0 = (m_confirmCursor == 0);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isSel0 ? 240 : 180);
        DrawBox(btn0X, btnY, btn0X + btnW, btnY + btnH, isSel0 ? GetColor(20, 50, 40) : GetColor(18, 28, 38), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(btn0X, btnY, btn0X + btnW, btnY + btnH, isSel0 ? GetColor(0, 255, 180) : GetColor(50, 90, 80), FALSE);
        if (isSel0) {
            DrawBox(btn0X + 1, btnY + 1, btn0X + btnW - 1, btnY + btnH - 1, GetColor(0, 255, 180), FALSE);
            DrawBox(btn0X, btnY, btn0X + 8, btnY + btnH, GetColor(0, 255, 180), TRUE);
        }
        FontManager::GetInstance().DrawStringCenter(btn0X + btnW / 2, btnY + 18, "▶  出撃開始 (START GAME)", isSel0 ? GetColor(255, 255, 255) : GetColor(180, 220, 200), 22);

        // ボタン1: 装備を変更 (CANCEL)
        bool isSel1 = (m_confirmCursor == 1);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isSel1 ? 240 : 180);
        DrawBox(btn1X, btnY, btn1X + btnW, btnY + btnH, isSel1 ? GetColor(50, 25, 30) : GetColor(28, 20, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(btn1X, btnY, btn1X + btnW, btnY + btnH, isSel1 ? GetColor(255, 100, 100) : GetColor(80, 50, 60), FALSE);
        if (isSel1) {
            DrawBox(btn1X + 1, btnY + 1, btn1X + btnW - 1, btnY + btnH - 1, GetColor(255, 100, 100), FALSE);
            DrawBox(btn1X, btnY, btn1X + 8, btnY + btnH, GetColor(255, 100, 100), TRUE);
        }
        FontManager::GetInstance().DrawStringCenter(btn1X + btnW / 2, btnY + 18, "←  装備を変更 (CANCEL)", isSel1 ? GetColor(255, 255, 255) : GetColor(220, 180, 180), 22);
    }
}

void WeaponSelectScene::Finalize() {
}
